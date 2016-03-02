/*
 * Copyright (c) 2011, 2012, 2013, 2014, 2015 Jonas 'Sortie' Termansen.
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 *
 * textterminal.cpp
 * Translates a character stream to a 2 dimensional array of characters.
 */

#include <stdio.h>
#include <string.h>
#include <wchar.h>

#include <sortix/vga.h>

#include <sortix/kernel/kernel.h>
#include <sortix/kernel/refcount.h>
#include <sortix/kernel/textbuffer.h>

#include "textterminal.h"

namespace Sortix {

static const uint16_t DEFAULT_COLOR = COLOR8_LIGHT_GREY | COLOR8_BLACK << 4;

TextTerminal::TextTerminal(TextBufferHandle* textbufhandle)
{
	memset(&ps, 0, sizeof(ps));
	this->textbufhandle = textbufhandle;
	this->termlock = KTHREAD_MUTEX_INITIALIZER;
	Reset();
}

TextTerminal::~TextTerminal()
{
}

void TextTerminal::Reset()
{
	next_attr = 0;
	vgacolor = DEFAULT_COLOR;
	column = line = 0;
	ansisavedposx = ansisavedposy = 0;
	ansimode = NONE;
	TextBuffer* textbuf = textbufhandle->Acquire();
	TextPos fillfrom(0, 0);
	TextPos fillto(textbuf->Width()-1, textbuf->Height()-1);
	TextChar fillwith(' ', vgacolor, 0);
	textbuf->Fill(fillfrom, fillto, fillwith);
	textbuf->SetCursorEnabled(true);
	UpdateCursor(textbuf);
	textbufhandle->Release(textbuf);
}

size_t TextTerminal::Print(const char* string, size_t stringlen)
{
	ScopedLock lock(&termlock);
	TextBuffer* textbuf = textbufhandle->Acquire();
	for ( size_t i = 0; i < stringlen; i++ )
		PutChar(textbuf, string[i]);
	UpdateCursor(textbuf);
	textbufhandle->Release(textbuf);
	return stringlen;
}

size_t TextTerminal::Width() const
{
	ScopedLock lock(&termlock);
	TextBuffer* textbuf = textbufhandle->Acquire();
	size_t width = textbuf->Width();
	textbufhandle->Release(textbuf);
	return width;
}

size_t TextTerminal::Height() const
{
	ScopedLock lock(&termlock);
	TextBuffer* textbuf = textbufhandle->Acquire();
	size_t height = textbuf->Height();
	textbufhandle->Release(textbuf);
	return height;
}

void TextTerminal::GetCursor(size_t* column, size_t* row) const
{
	ScopedLock lock(&termlock);
	*column = this->column;
	*row = this->line;
}

bool TextTerminal::Sync()
{
	// Reading something from the textbuffer may cause it to block while
	// finishing rendering, effectively synchronizing with it.
	ScopedLock lock(&termlock);
	TextBuffer* textbuf = textbufhandle->Acquire();
	textbuf->GetCursorPos();
	textbufhandle->Release(textbuf);
	return true;
}

bool TextTerminal::Invalidate()
{
	ScopedLock lock(&termlock);
	TextBuffer* textbuf = textbufhandle->Acquire();
	textbuf->Invalidate();
	textbufhandle->Release(textbuf);
	return true;
}

bool TextTerminal::EmergencyIsImpaired()
{
	// This is during a kernel emergency where preemption has been disabled and
	// this is the only thread running.

	if ( !kthread_mutex_trylock(&termlock) )
		return true;
	kthread_mutex_unlock(&termlock);

	if ( textbufhandle->EmergencyIsImpaired() )
		return true;

	TextBuffer* textbuf = textbufhandle->EmergencyAcquire();
	bool textbuf_was_impaired = textbuf->EmergencyIsImpaired();
	textbufhandle->EmergencyRelease(textbuf);
	if ( textbuf_was_impaired )
		return true;

	return false;
}

bool TextTerminal::EmergencyRecoup()
{
	// This is during a kernel emergency where preemption has been disabled and
	// this is the only thread running.

	if ( !kthread_mutex_trylock(&termlock) )
		return false;
	kthread_mutex_unlock(&termlock);

	if ( textbufhandle->EmergencyIsImpaired() &&
	     !textbufhandle->EmergencyRecoup() )
		return false;

	TextBuffer* textbuf = textbufhandle->EmergencyAcquire();
	bool textbuf_failure = textbuf->EmergencyIsImpaired() &&
	                       !textbuf->EmergencyRecoup();
	textbufhandle->EmergencyRelease(textbuf);

	if ( !textbuf_failure )
		return false;

	return true;
}

void TextTerminal::EmergencyReset()
{
	// This is during a kernel emergency where preemption has been disabled and
	// this is the only thread running.

	textbufhandle->EmergencyReset();

	TextBuffer* textbuf = textbufhandle->EmergencyAcquire();
	textbuf->EmergencyReset();
	textbufhandle->EmergencyRelease(textbuf);

	this->termlock = KTHREAD_MUTEX_INITIALIZER;
	Reset();
}

size_t TextTerminal::EmergencyPrint(const char* string, size_t stringlen)
{
	// This is during a kernel emergency where preemption has been disabled and
	// this is the only thread running. Another thread may have been interrupted
	// while it held the terminal lock. The best case is if the terminal lock is
	// currently unused, which would mean everything is safe.

	TextBuffer* textbuf = textbufhandle->EmergencyAcquire();
	for ( size_t i = 0; i < stringlen; i++ )
		PutChar(textbuf, string[i]);
	UpdateCursor(textbuf);
	textbufhandle->EmergencyRelease(textbuf);
	return stringlen;
}

size_t TextTerminal::EmergencyWidth() const
{
	// This is during a kernel emergency where preemption has been disabled and
	// this is the only thread running. Another thread may have been interrupted
	// while it held the terminal lock. The best case is if the terminal lock is
	// currently unused, which would mean everything is safe.

	TextBuffer* textbuf = textbufhandle->EmergencyAcquire();
	size_t width = textbuf->Width();
	textbufhandle->EmergencyRelease(textbuf);
	return width;
}

size_t TextTerminal::EmergencyHeight() const
{
	// This is during a kernel emergency where preemption has been disabled and
	// this is the only thread running. Another thread may have been interrupted
	// while it held the terminal lock. The best case is if the terminal lock is
	// currently unused, which would mean everything is safe.

	TextBuffer* textbuf = textbufhandle->EmergencyAcquire();
	size_t height = textbuf->Height();
	textbufhandle->EmergencyRelease(textbuf);
	return height;
}

void TextTerminal::EmergencyGetCursor(size_t* column, size_t* row) const
{
	// This is during a kernel emergency where preemption has been disabled and
	// this is the only thread running. Another thread may have been interrupted
	// while it held the terminal lock. The best case is if the terminal lock is
	// currently unused, which would mean everything is safe.

	*column = this->column;
	*row = this->line;
}

bool TextTerminal::EmergencySync()
{
	// This is during a kernel emergency where preemption has been disabled and
	// this is the only thread running. There is no need to synchronize the
	// text buffer here as there is no background thread rendering the console.

	return true;
}

void TextTerminal::PutChar(TextBuffer* textbuf, char c)
{
	if ( ansimode )
		return PutAnsiEscaped(textbuf, c);

	if ( mbsinit(&ps) )
	{
		switch ( c )
		{
		case '\n': Newline(textbuf); return;
		case '\r': column = 0; return;
		case '\b': Backspace(textbuf); return;
		case '\t': Tab(textbuf); return;
		case '\e': AnsiReset(); return;
		case 127: return;
		default: break;
		}
	}

	wchar_t wc;
	size_t result = mbrtowc(&wc, &c, 1, &ps);
	if ( result == (size_t) -2 )
		return;
	if ( result == (size_t) -1 )
	{
		memset(&ps, 0, sizeof(ps));
		wc = L'�';
	}
	if ( result == (size_t) 0 )
		wc = L' ';

	if ( textbuf->Width() <= column )
		Newline(textbuf);
	TextPos pos(column++, line);
	TextChar tc(wc, vgacolor, ATTR_CHAR | next_attr);
	textbuf->SetChar(pos, tc);
	next_attr = 0;
}

void TextTerminal::UpdateCursor(TextBuffer* textbuf)
{
	textbuf->SetCursorPos(TextPos(column, line));
}

void TextTerminal::Newline(TextBuffer* textbuf)
{
	TextPos pos(column, line);
	TextChar tc = textbuf->GetChar(pos);
	if ( !(tc.attr & ATTR_CHAR) )
	{
		tc.attr |= ATTR_CHAR;
		textbuf->SetChar(pos, tc);
	}
	column = 0;
	if ( line < textbuf->Height()-1 )
		line++;
	else
	{
		textbuf->Scroll(1, TextChar(' ', vgacolor, 0));
		line = textbuf->Height()-1;
	}
}

static TextPos DecrementTextPos(TextBuffer* textbuf, TextPos pos)
{
	if ( !pos.x && !pos.y )
		return pos;
	if ( !pos.x )
		return TextPos(textbuf->Width(), pos.y-1);
	return TextPos(pos.x-1, pos.y);
}

void TextTerminal::Backspace(TextBuffer* textbuf)
{
	TextPos pos(column, line);
	while ( pos.x || pos.y )
	{
		pos = DecrementTextPos(textbuf, pos);
		TextChar tc = textbuf->GetChar(pos);
		next_attr = tc.attr & (ATTR_BOLD | ATTR_UNDERLINE);
		if ( tc.c == L'_' )
			next_attr |= ATTR_UNDERLINE;
		else if ( tc.c == L' ' )
			next_attr &= ~(ATTR_BOLD | ATTR_CHAR);
		else
			next_attr |= ATTR_BOLD;
		textbuf->SetChar(pos, TextChar(' ', vgacolor, 0));
		if ( tc.attr & ATTR_CHAR )
			break;
	}
	column = pos.x;
	line = pos.y;
}

void TextTerminal::Tab(TextBuffer* textbuf)
{
	if ( column == textbuf->Width() )
		Newline(textbuf);
	unsigned int count = 8 - (column % 8);
	for ( unsigned int i = 0; i < count; i++ )
	{
		if ( column == textbuf->Width() )
			break;
		TextPos pos(column++, line);
		TextChar tc(' ', vgacolor, i == 0 ? ATTR_CHAR : 0);
		textbuf->SetChar(pos, tc);
	}
}

// TODO: This implementation of the 'Ansi Escape Codes' is incomplete and hacky.
void TextTerminal::AnsiReset()
{
	next_attr = 0;
	ansiusedparams = 0;
	currentparamindex = 0;
	ansiparams[0] = 0;
	paramundefined = true;
	ignoresequence = false;
	ansimode = CSI;
}

void TextTerminal::PutAnsiEscaped(TextBuffer* textbuf, char c)
{
	// Check the proper prefixes are used.
	if ( ansimode == CSI )
	{
		if ( c != '[' ) { ansimode = NONE; return; }
		ansimode = COMMAND;
		return;
	}

	// Read part of a parameter.
	if ( '0' <= c && c <= '9' )
	{
		if ( paramundefined )
			ansiusedparams++;
		paramundefined = false;
		unsigned val = c - '0';
		ansiparams[currentparamindex] *= 10;
		ansiparams[currentparamindex] += val;
	}

	// Parameter delimiter.
	else if ( c == ';' )
	{
		if ( currentparamindex == ANSI_NUM_PARAMS - 1 )
		{
			ansimode = NONE;
			return;
		}
		paramundefined = true;
		ansiparams[++currentparamindex] = 0;
	}

	// Left for future standardization, so discard this sequence.
	else if ( c == ':' )
	{
		ignoresequence = true;
	}

	// Run a command.
	else if ( 64 <= c && c <= 126 )
	{
		if ( !ignoresequence )
			RunAnsiCommand(textbuf, c);
	}

	// Something I don't understand, and ignore intentionally.
	else if ( c == '?' )
	{
	}

	// TODO: There are some rare things that should be supported here.

	// Ignore unknown input.
	else
	{
		ansimode = NONE;
	}
}

void TextTerminal::RunAnsiCommand(TextBuffer* textbuf, char c)
{
	const unsigned width = (unsigned) textbuf->Width();
	const unsigned height = (unsigned) textbuf->Height();

	switch ( c )
	{
	case 'A': // Cursor up
	{
		unsigned dist = 0 < ansiusedparams ? ansiparams[0] : 1;
		if ( line < dist )
			line = 0;
		else
			line -= dist;
	} break;
	case 'B': // Cursor down
	{
		unsigned dist = 0 < ansiusedparams ? ansiparams[0] : 1;
		if ( height <= line + dist )
			line = height-1;
		else
			line += dist;
	} break;
	case 'C': // Cursor forward
	{
		unsigned dist = 0 < ansiusedparams ? ansiparams[0] : 1;
		if ( width <= column + dist )
			column = width-1;
		else
			column += dist;
	} break;
	case 'D': // Cursor backward
	{
		unsigned dist = 0 < ansiusedparams ? ansiparams[0] : 1;
		if ( column < dist )
			column = 0;
		else
			column -= dist;
	} break;
	case 'E': // Move to beginning of line N lines down.
	{
		column = 0;
		unsigned dist = 0 < ansiusedparams ? ansiparams[0] : 1;
		if ( height <= line + dist )
			line = height-1;
		else
			line += dist;
	} break;
	case 'F': // Move to beginning of line N lines up.
	{
		column = 0;
		unsigned dist = 0 < ansiusedparams ? ansiparams[0] : 1;
		if ( line < dist )
			line = 0;
		else
			line -= dist;
	} break;
	case 'G': // Move the cursor to column N.
	{
		unsigned pos = 0 < ansiusedparams ? ansiparams[0]-1 : 0;
		if ( width <= pos )
			pos = width-1;
		column = pos;
	} break;
	case 'H': // Move the cursor to line Y, column X.
	case 'f':
	{
		unsigned posy = 0 < ansiusedparams ? ansiparams[0]-1 : 0;
		unsigned posx = 1 < ansiusedparams ? ansiparams[1]-1 : 0;
		if ( width <= posx )
			posx = width-1;
		if ( height <= posy )
			posy = height-1;
		column = posx;
		line = posy;
	} break;
	case 'J': // Erase parts of the screen.
	{
		unsigned mode = 0 < ansiusedparams ? ansiparams[0] : 0;
		TextPos from(0, 0);
		TextPos to(0, 0);

		if ( mode == 0 ) // From cursor to end.
			from = TextPos{column, line},
			to = TextPos{width-1, height-1};

		if ( mode == 1 ) // From start to cursor.
			from = TextPos{0, 0},
			to = TextPos{column, line};

		if ( mode == 2 ) // Everything.
			from = TextPos{0, 0},
			to = TextPos{width-1, height-1};

		textbuf->Fill(from, to, TextChar(' ', vgacolor, 0));
	} break;
	case 'K': // Erase parts of the current line.
	{
		unsigned mode = 0 < ansiusedparams ? ansiparams[0] : 0;
		TextPos from(0, 0);
		TextPos to(0, 0);

		if ( mode == 0 ) // From cursor to end.
			from = TextPos{column, line},
			to = TextPos{width-1, line};

		if ( mode == 1 ) // From start to cursor.
			from = TextPos{0, line},
			to = TextPos{column, line};

		if ( mode == 2 ) // Everything.
			from = TextPos{0, line},
			to = TextPos{width-1, line};

		textbuf->Fill(from, to, TextChar(' ', vgacolor, 0));
	} break;
	case 'S': // Scroll a line up and place a new line at the buttom.
	{
		textbuf->Scroll(1, TextChar(' ', vgacolor, 0));
		line = height-1;
	} break;
	case 'T': // Scroll a line up and place a new line at the top.
	{
		textbuf->Scroll(-1, TextChar(' ', vgacolor, 0));
		line = 0;
	} break;
	case 'm': // Change how the text is rendered.
	{
		if ( ansiusedparams == 0 )
		{
			ansiparams[0] = 0;
			ansiusedparams++;
		}

		// Convert from the ANSI color scheme to the VGA color scheme.
		const unsigned conversion[8] =
		{
			COLOR8_BLACK, COLOR8_RED, COLOR8_GREEN, COLOR8_BROWN,
			COLOR8_BLUE, COLOR8_MAGENTA, COLOR8_CYAN, COLOR8_LIGHT_GREY,
		};

		for ( size_t i = 0; i < ansiusedparams; i++ )
		{
			unsigned cmd = ansiparams[i];
			// Turn all attributes off.
			if ( cmd == 0 )
			{
				vgacolor = DEFAULT_COLOR;
			}
			// Set text color.
			else if ( 30 <= cmd && cmd <= 37 )
			{
				unsigned val = cmd - 30;
				vgacolor &= 0xF0;
				vgacolor |= conversion[val] << 0;
			}
			// Set default text color.
			else if ( cmd == 39 )
			{
				vgacolor &= 0xF0;
				vgacolor |= DEFAULT_COLOR & 0x0F;
			}
			// Set background color.
			else if ( 40 <= cmd && cmd <= 47 )
			{
				unsigned val = cmd - 40;
				vgacolor &= 0x0F;
				vgacolor |= conversion[val] << 4;
			}
			// Set default background color.
			else if ( cmd == 49 )
			{
				vgacolor &= 0x0F;
				vgacolor |= DEFAULT_COLOR & 0xF0;
			}
			// Set text color.
			else if ( 90 <= cmd && cmd <= 97 )
			{
				unsigned val = cmd - 90;
				vgacolor &= 0xF0;
				vgacolor |= (0x8 | conversion[val]) << 0;
			}
			// Set background color.
			else if ( 100 <= cmd && cmd <= 107 )
			{
				unsigned val = cmd - 100;
				vgacolor &= 0x0F;
				vgacolor |= (0x8 | conversion[val]) << 4;
			}
			// TODO: There are many other things we don't support.
		}
	} break;
	case 'n': // Request special information from terminal.
	{
		// TODO: Handle this code.
	} break;
	case 's': // Save cursor position.
	{
		ansisavedposx = column;
		ansisavedposx = line;
	} break;
	case 'u': // Restore cursor position.
	{
		column = ansisavedposx;
		line = ansisavedposx;
		if ( width <= column )
			column = width-1;
		if ( height <= line )
			line = height-1;
	} break;
	case 'l': // Hide cursor.
	{
		// TODO: This is somehow related to the special char '?'.
		if ( 0 < ansiusedparams && ansiparams[0] == 25 )
			textbuf->SetCursorEnabled(false);
	} break;
	case 'h': // Show cursor.
	{
		// TODO: This is somehow related to the special char '?'.
		if ( 0 < ansiusedparams && ansiparams[0] == 25 )
			textbuf->SetCursorEnabled(true);
	} break;
	// TODO: Handle other cases.
	}

	ansimode = NONE;
}

} // namespace Sortix

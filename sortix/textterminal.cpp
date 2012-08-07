/*******************************************************************************

    Copyright(C) Jonas 'Sortie' Termansen 2011, 2012.

    This file is part of Sortix.

    Sortix is free software: you can redistribute it and/or modify it under the
    terms of the GNU General Public License as published by the Free Software
    Foundation, either version 3 of the License, or (at your option) any later
    version.

    Sortix is distributed in the hope that it will be useful, but WITHOUT ANY
    WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
    FOR A PARTICULAR PURPOSE. See the GNU General Public License for more
    details.

    You should have received a copy of the GNU General Public License along with
    Sortix. If not, see <http://www.gnu.org/licenses/>.

    textterminal.cpp
    Translates a character stream to a 2 dimensional array of character.

*******************************************************************************/

#include <sortix/kernel/platform.h>
#include <sortix/kernel/refcount.h>
#include <sortix/kernel/textbuffer.h>
#include <sortix/vga.h>
#include "textterminal.h"

namespace Sortix {

const uint16_t DEFAULT_COLOR = COLOR8_LIGHT_GREY << 0U | COLOR8_BLACK << 4U;
const uint16_t ATTR_CHAR = 1U << 0U;

TextTerminal::TextTerminal(Ref<TextBufferHandle> textbufhandle)
{
	this->textbufhandle = textbufhandle;
	this->termlock = KTHREAD_MUTEX_INITIALIZER;
	Reset();
}

TextTerminal::~TextTerminal()
{
}

void TextTerminal::Reset()
{
	vgacolor = DEFAULT_COLOR;
	column = line = 0;
	ansisavedposx = ansisavedposy = 0;
	ansimode = NONE;
	TextBuffer* textbuf = textbufhandle->Acquire();
	TextPos fillfrom(0, 0);
	TextPos fillto(textbuf->Width()-1, textbuf->Height()-1);
	TextChar fillwith(' ', vgacolor);
	textbuf->Fill(fillfrom, fillto, fillwith, 0);
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

void TextTerminal::PutChar(TextBuffer* textbuf, char c)
{
	if ( ansimode )
		PutAnsiEscaped(textbuf, c);
	else switch ( c )
	{
	case '\n': Newline(textbuf); break;
	case '\r': column = 0; break;
	case '\b': Backspace(textbuf); break;
	case '\t': Tab(textbuf); break;
	case '\e': AnsiReset(); break;
	default:
	{
		if ( textbuf->Width() <= column )
			Newline(textbuf);
		TextPos pos(column++, line);
		TextChar tc(c, vgacolor);
		textbuf->SetChar(pos, tc);
		textbuf->SetCharAttr(pos, ATTR_CHAR);
	} break;
	}
}

void TextTerminal::UpdateCursor(TextBuffer* textbuf)
{
	textbuf->SetCursorPos(TextPos(column, line));
}

void TextTerminal::Newline(TextBuffer* textbuf)
{
	textbuf->SetCharAttr(TextPos(column, line), ATTR_CHAR);
	column = 0;
	if ( line < textbuf->Height()-1 )
		line++;
	else
		textbuf->Scroll(1, TextChar(' ', vgacolor)),
		line = textbuf->Height()-1;
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
		uint16_t attr = textbuf->GetCharAttr(pos);
		textbuf->SetChar(pos, TextChar(' ', vgacolor));
		textbuf->SetCharAttr(pos, attr & ~ATTR_CHAR);
		if ( attr & ATTR_CHAR )
			break;
	}
	column = pos.x;
	line = pos.y;
}

void TextTerminal::Tab(TextBuffer* textbuf)
{
	if ( column == textbuf->Width() )
		Newline(textbuf);
	// TODO: This does not work correctly if the text buffer width is not a
	// multiple of four and the column is near the edge.
	unsigned until = 8 - (column % 8);
	textbuf->SetCharAttr(TextPos(column, line), ATTR_CHAR);
	while ( (until--) != 0 )
		textbuf->SetChar(TextPos(column++, line), TextChar(' ', vgacolor));
}

// TODO: This implementation of the 'Ansi Escape Codes' is incomplete and hacky.
void TextTerminal::AnsiReset()
{
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

		textbuf->Fill(from, to, TextChar(' ', vgacolor), 0);
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

		textbuf->Fill(from, to, TextChar(' ', vgacolor), 0);
	} break;
	case 'S': // Scroll a line up and place a new line at the buttom.
	{
		textbuf->Scroll(1, TextChar(' ', vgacolor));
		line = height-1;
	} break;
	case 'T': // Scroll a line up and place a new line at the top.
	{
		textbuf->Scroll(-1, TextChar(' ', vgacolor));
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
			// Set background color.
			else if ( 40 <= cmd && cmd <= 47 )
			{
				unsigned val = cmd - 40;
				vgacolor &= 0x0F;
				vgacolor |= conversion[val] << 4;
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

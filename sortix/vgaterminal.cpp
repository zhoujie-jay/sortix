/******************************************************************************

	COPYRIGHT(C) JONAS 'SORTIE' TERMANSEN 2011.

	This file is part of Sortix.

	Sortix is free software: you can redistribute it and/or modify it under the
	terms of the GNU General Public License as published by the Free Software
	Foundation, either version 3 of the License, or (at your option) any later
	version.

	Sortix is distributed in the hope that it will be useful, but WITHOUT ANY
	WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
	FOR A PARTICULAR PURPOSE. See the GNU General Public License for more
	details.

	You should have received a copy of the GNU General Public License along
	with Sortix. If not, see <http://www.gnu.org/licenses/>.

	vgaterminal.cpp
	A terminal based on the VGA text mode buffer.

******************************************************************************/

#include <sortix/kernel/platform.h>
#include <sortix/kernel/log.h>
#include <sortix/vga.h>
#include <libmaxsi/memory.h>
#include "vga.h"
#include "vgaterminal.h"

using namespace Maxsi;

namespace Sortix
{
	namespace VGATerminal
	{
		const nat width = 80;
		const nat height = 25;
		const uint16_t defaultcolor = (COLOR8_LIGHT_GREY << 8) | (COLOR8_BLACK << 12);
		uint16_t* const vga = (uint16_t* const) 0xB8000;
		uint16_t vgaattr[width * height];
		const uint16_t VGAATTR_CHAR = (1U<<0U);
		nat line;
		nat column;
		uint16_t currentcolor;
		nat ansisavedposx;
		nat ansisavedposy;
		bool showcursor;

		enum
		{
			NONE,
			CSI,
			COMMAND,
		} ansimode;

		void UpdateCursor()
		{
			if ( showcursor )
			{
				VGA::SetCursor(column, line);
			}
			else
			{
				VGA::SetCursor(width, height-1);
			}
		}

		// Clear the screen, put the cursor at the top left corner, set default
		// text color, and reset ANSI escape sequence state.
		void Reset()
		{
			ansimode = NONE;

			line = 0;
			column = 0;
			ansisavedposx = 0;
			ansisavedposy = 0;

			currentcolor = defaultcolor;

			for ( nat y = 0; y < height; y++ )
			{
				for ( nat x = 0; x < width; x++ )
				{
					unsigned index = y * width + x;
					vga[index] = ' ' | defaultcolor;
					vgaattr[index] = 0;
				}
			}

			// Reset the VGA cursor.
			showcursor = true;
			UpdateCursor();
		}

		// Initialize the terminal driver.
		void Init()
		{
			Reset();
		}

		// Move every line one row up and leaves an empty line at the bottom.
		void ScrollUp()
		{
			for ( nat y = 1; y < height; y++ )
			{
				size_t linesize = width * sizeof(uint16_t);
				size_t fromoff = (y-0) * width;
				size_t destoff = (y-1) * width;
				Memory::Copy(vga + destoff, vga + fromoff, linesize);
				Memory::Copy(vgaattr + destoff, vgaattr + fromoff, linesize);
			}

			for ( nat x = 0; x < width; x++ )
			{
				unsigned index = (height-1) * width + x;
				vga[index] =  ' ' | currentcolor;
				vgaattr[index] = 0;
			}
		}

		// Move every line one row down and leaves an empty line at the top.
		void ScrollDown()
		{
			for ( nat y = 1; y < height; y++ )
			{
				size_t linesize = width * sizeof(uint16_t);
				size_t fromoff = (y-1) * width;
				size_t destoff = (y-0) * width;
				Memory::Copy(vga + destoff, vga + fromoff, linesize);
				Memory::Copy(vgaattr + destoff, vgaattr + fromoff, linesize);
			}

			for ( nat x = 0; x < width; x++ )
			{
				unsigned index = x;
				vga[index] =  ' ' | currentcolor;
				vgaattr[index] = 0;
			}
		}

		// Move to the next line. If at bottom, scroll one line up.
		void Newline()
		{
			vgaattr[line * width + column] |= VGAATTR_CHAR;
			if ( line < height - 1 )
			{
				line++; column = 0;
			}
			else
			{
				ScrollUp(); column = 0;
			}

			UpdateCursor();
		}

		void ANSIReset();
		void ParseANSIEscape(char c);

		// Print text to the vga framebuffer, simulating how a terminal would
		// display the text.
		size_t Print(void* /*user*/, const char* string, size_t stringlen)
		{
			// Check for the bug where string contains the address 0x80000000
			// which is not legal to print. It looks like we are trying to
			// print a string from the stack that wasn't NUL-terminated. I
			// tracked down the string value comes from LogTerminal, but it
			// doesn't seem to originate from a write(2) call. Weird stuff.
			addr_t straddr = (addr_t) string;
			if ( straddr <= 0x80000000UL && 0x80000000UL <= straddr + stringlen )
			{
				PanicF("Trying to print bad string 0x%zx + 0x%zx bytes: this "
				       "is a known bug, but with an unknown cause.", straddr,
				        stringlen);
			}

			// Iterate over each character.
			size_t left = stringlen;
			while ( (left--) > 0 )
			{
				char c = *(string++);

				// If we are parsing an escape sequence, parse another char.
				if ( ansimode != NONE ) { ParseANSIEscape(c); continue; }

				switch ( c )
				{
					// Move cursor to the next line and scroll up (if needed).
					case '\n':
					{
						Newline();
						break;
					}
					// Move cursor to start of line.
					case '\r':
					{
						column = 0;
						break;
					}
					// Delete the previous character.
					case '\b':
					{
						unsigned pos = line * width + column;
						while ( pos )
						{
							unsigned nextpos = pos-1;
							vga[nextpos] = ' ' | currentcolor;
							pos = nextpos;
							uint16_t attr = vgaattr[pos];
							vgaattr[pos] = 0;
							if ( attr & VGAATTR_CHAR ) { break; }
						}
						column = pos % width;
						line = pos / width;
						break;
					}
					// Expand a tab to a few spaces.
					case '\t':
					{
						if ( column == width ) { Newline(); }
						nat until = 4 - (column % 4);

						vgaattr[line * width + (column)] |= VGAATTR_CHAR;
						while ( (until--) != 0 )
						{
							vga[line * width + (column++)] = ' ' | currentcolor;
						}
						break;
					}
					// Initialize an ANSI escape sequence, allowing much more
					// powerful control of the terminal than ASCII does.
					case '\e':
					{
						ANSIReset();
						break;
					}
					// Print this character at the cursor and increment the cursor.
					default:
					{
						if ( column == width ) { Newline(); }
						unsigned index = line * width + (column++);
						vga[index] = c | currentcolor;
						vgaattr[index] |= VGAATTR_CHAR;
						break;
					}
				}
			}

			// Update the VGA hardware cursor.
			UpdateCursor();

			return stringlen;
		}

		const size_t ANSI_NUM_PARAMS = 16;
		nat ansiusedparams;
		nat ansiparams[ANSI_NUM_PARAMS];
		nat currentparamindex;
		bool paramundefined;
		bool ignoresequence;

		// TODO: The ANSI escape code is only partially implemented!

		// Begin an ANSI esacpe sequence.
		void ANSIReset()
		{
			if ( ansimode == NONE )
			{
				ansiusedparams = 0;
				currentparamindex = 0;
				ansiparams[0] = 0;
				paramundefined = true;
				ignoresequence = false;
				ansimode = CSI;
			}
		}

		// Reads parameters and changes font properties accordingly.
		void AnsiSetFont()
		{
			// Convert from the ANSI color scheme to the VGA color scheme.
			nat conversion[8] = { COLOR8_BLACK, COLOR8_RED, COLOR8_GREEN, COLOR8_BROWN,
			                      COLOR8_BLUE, COLOR8_MAGENTA, COLOR8_CYAN, COLOR8_LIGHT_GREY };

			for ( size_t i = 0; i < ansiusedparams; i++ )
			{
				nat cmd = ansiparams[i];
				// Turn all attributes off.
				if ( cmd == 0 )
				{
					currentcolor = defaultcolor;
				}
				// Set text color.
				else if ( 30 <= cmd && cmd <= 37 )
				{
					nat val = cmd - 30;
					currentcolor &= (0xF0FF);
					currentcolor |= (conversion[val] << 8);
				}
				// Set background color.
				else if ( 40 <= cmd && cmd <= 47 )
				{
					nat val = cmd - 40;
					currentcolor &= (0x0FFF);
					currentcolor |= (conversion[val] << 12);
				}
				// TODO: There are many other things we don't support.
			}
		}

		// Executes an ASNI escape command based on given parameters.
		void RunANSICommand(char c)
		{
			switch ( c )
			{
				// Cursor up
				case 'A':
				{
					nat dist = ( 0 < ansiusedparams ) ? ansiparams[0] : 1;
					if ( line < dist ) { line = 0; } else { line -= dist; }
					break;
				}
				// Cursor down
				case 'B':
				{
					nat dist = ( 0 < ansiusedparams ) ? ansiparams[0] : 1;
					if ( height <= line + dist ) { line = height-1; } else { line += dist; }
					break;
				}
				// Cursor forward
				case 'C':
				{
					nat dist = ( 0 < ansiusedparams ) ? ansiparams[0] : 1;
					if ( width <= column + dist ) { column = width-1; } else { column += dist; }
					break;
				}
				// Cursor backward
				case 'D':
				{
					nat dist = ( 0 < ansiusedparams ) ? ansiparams[0] : 1;
					if ( column < dist ) { column = 0; } else { column -= dist; }
					break;
				}
				// Move to beginning of line N lines down.
				case 'E':
				{
					column = 0;
					nat dist = ( 0 < ansiusedparams ) ? ansiparams[0] : 1;
					if ( height <= line + dist ) { line = height-1; } else { line += dist; }
					break;
				}
				// Move to beginning of line N lines up.
				case 'F':
				{
					column = 0;
					nat dist = ( 0 < ansiusedparams ) ? ansiparams[0] : 1;
					if ( line < dist ) { line = 0; } else { line -= dist; }
					break;
				}
				// Move the cursor to column N.
				case 'G':
				{
					nat pos = ( 0 < ansiusedparams ) ? ansiparams[0]-1 : 0;
					if ( width <= pos ) { pos = width-1; }
					column = pos;
					break;
				}
				// Move the cursor to line Y, column X.
				case 'H':
				case 'f':
				{
					nat posy = ( 0 < ansiusedparams ) ? ansiparams[0]-1 : 0;
					nat posx = ( 1 < ansiusedparams ) ? ansiparams[1]-1 : 0;
					if ( width <= posx ) { posx = width-1; }
					if ( height <= posy ) { posy = height-1; }
					column = posx;
					line = posy;
					break;
				}
				// Erase parts of the screen.
				case 'J':
				{
					nat mode = ( 0 < ansiusedparams ) ? ansiparams[0] : 0;

					nat from = 0;
					nat to = 0;

					// From cursor to end.
					if ( mode == 0 ) { from = line*width + column; to = height*width - 1; }

					// From start to cursor.
					if ( mode == 1 ) { from = 0; to = line*width + column; }

					// Everything.
					if ( mode == 2 ) { from = 0; to = height*width - 1; }

					for ( nat i = from; i <= to; i++ )
					{
						vga[i] = ' ' | currentcolor;
						vgaattr[i] = 0;
					}

					break;
				}
				// Erase parts of the current line.
				case 'K':
				{
					nat mode = ( 0 < ansiusedparams ) ? ansiparams[0] : 0;

					nat from = 0;
					nat to = 0;

					// From cursor to end.
					if ( mode == 0 ) { from = column; to = width - 1; }

					// From start to cursor.
					if ( mode == 1 ) { from = 0; to = column; }

					// Everything.
					if ( mode == 2 ) { from = 0; to = width - 1; }

					for ( nat i = from; i <= to; i++ )
					{
						unsigned index = line * width + i;
						vga[index] = ' ' | currentcolor;
						vgaattr[index] = 0;
					}

					break;
				}
				// Scroll a line up and place a new line at the buttom.
				case 'S':
				{
					ScrollUp(); line = height-1;
					break;
				}
				// Scroll a line up and place a new line at the top.
				case 'T':
				{
					ScrollDown(); line = 0;
					break;
				}
				// Change how the text is rendered.
				case 'm':
				{
					if ( ansiusedparams == 0 )
					{
						ansiparams[0] = 0;
						ansiusedparams++;
					}
					AnsiSetFont();
					break;
				}
				// Request special information from terminal.
				case 'n':
				{
					// TODO: Handle this code.
					break;
				}
				// Save cursor position.
				case 's':
				{
					ansisavedposx = column;
					ansisavedposx = line;
					break;
				}
				// Restore cursor position.
				case 'u':
				{
					column = ansisavedposx;
					line = ansisavedposx;
					if ( width <= column ) { column = width-1; }
					if ( height <= line ) { line = height-1; }
					break;
				}
				// Hide cursor.
				case 'l':
				{
					// TODO: This is somehow related to the special char '?'.
					if ( 0 < ansiusedparams && ansiparams[0] == 25 )
					{
						showcursor = false;
						UpdateCursor();
					}
					break;
				}
				// Show cursor.
				case 'h':
				{
					// TODO: This is somehow related to the special char '?'.
					if ( 0 < ansiusedparams && ansiparams[0] == 25 )
					{
						showcursor = true;
						UpdateCursor();
					}
					break;
				}
				// TODO: Handle other cases.
			}

			ansimode = NONE;
		}

		// Parse another char of an ASNI escape code.
		void ParseANSIEscape(char c)
		{
			// Check the proper prefixes are used.
			if ( ansimode == CSI )
			{
				if ( c != '[' ) { ansimode = NONE; return; }
				ansimode = COMMAND;
			}

			// Parse parameters and the command.
			else if ( ansimode == COMMAND )
			{
				// Read part of a parameter.
				if ( '0' <= c && c <= '9' )
				{
					if ( paramundefined ) { ansiusedparams++; }
					paramundefined = false;
					nat val = c - '0';
					ansiparams[currentparamindex] *= 10;
					ansiparams[currentparamindex] += val;
				}

				// Parameter delimiter.
				else if ( c == ';' )
				{
					if ( currentparamindex == ANSI_NUM_PARAMS - 1 ) { ansimode = NONE; return; }
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
					if ( !ignoresequence ) { RunANSICommand(c); }
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
		}
	}
}

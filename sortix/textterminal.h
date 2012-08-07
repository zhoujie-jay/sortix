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

    textterminal.h
    An indexable text buffer with the VGA text mode framebuffer as backend.

*******************************************************************************/

#ifndef SORTIX_TEXTTERMINAL_H
#define SORTIX_TEXTTERMINAL_H

#include <sortix/kernel/kthread.h>
#include <sortix/kernel/refcount.h>

namespace Sortix {

class TextBufferHandle;

class TextTerminal //: public Printable ?
{
public:
	TextTerminal(Ref<TextBufferHandle> textbufhandle);
	~TextTerminal();
	size_t Print(const char* string, size_t stringlen);
	size_t Width() const;
	size_t Height() const;

private:
	void PutChar(TextBuffer* textbuf, char c);
	void UpdateCursor(TextBuffer* textbuf);
	void Newline(TextBuffer* textbuf);
	void Backspace(TextBuffer* textbuf);
	void Tab(TextBuffer* textbuf);
	void PutAnsiEscaped(TextBuffer* textbuf, char c);
	void RunAnsiCommand(TextBuffer* textbuf, char c);
	void AnsiReset();
	void Reset();

private:
	mutable Ref<TextBufferHandle> textbufhandle;
	mutable kthread_mutex_t termlock;
	uint8_t vgacolor;
	unsigned column;
	unsigned line;
	unsigned ansisavedposx;
	unsigned ansisavedposy;
	enum { NONE = 0, CSI, COMMAND, } ansimode;
	static const size_t ANSI_NUM_PARAMS = 16;
	unsigned ansiusedparams;
	unsigned ansiparams[ANSI_NUM_PARAMS];
	unsigned currentparamindex;
	bool paramundefined;
	bool ignoresequence;

};

} // namespace Sortix

#endif

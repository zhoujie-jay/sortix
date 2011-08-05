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

	shell.h
	A very basic command line shell.

******************************************************************************/

#ifndef SORTIX_SHELL_H
#define SORTIX_SHELL_H

namespace Sortix
{
	class Shell : public IKeystrokable
	{
	protected:
		const static nat Width = 80;
		const static nat BacklogHeight = 100;
		const static nat Height = 25;
		char Backlog[Width * BacklogHeight];
		char Command[Width * Height];
		nat BacklogUsed;
		nat Column;
		nat Scroll;

	public:
		virtual void Init();

	private:
		void ClearScreen();

	public:
		void UpdateScreen();

	public:
		virtual void OnKeystroke(uint32_t CodePoint, bool KeyUp);

	public:
		void SetCursor(nat X, nat Y);
	};

	DECLARE_GLOBAL_OBJECT(Shell, Shell);
}

#endif


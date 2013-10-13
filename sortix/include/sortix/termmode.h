/******************************************************************************

	COPYRIGHT(C) JONAS 'SORTIE' TERMANSEN 2012.

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

	termmode.h
	Defines constants for various terminal modes.

******************************************************************************/

#ifndef SORTIX_TERMMODE_H
#define SORTIX_TERMMODE_H

#define TERMMODE_KBKEY (1U<<0U)
#define TERMMODE_UNICODE (1U<<1U)
#define TERMMODE_SIGNAL (1U<<2U)
#define TERMMODE_UTF8 (1U<<3U)
#define TERMMODE_LINEBUFFER (1U<<4U)
#define TERMMODE_ECHO (1U<<5U)
#define TERMMODE_NONBLOCK (1U<<6U)

#endif

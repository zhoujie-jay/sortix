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

	globals.h
	Macros useful to declare and define global objects in Sortix.

******************************************************************************/

#ifndef SORTIX_GLOBALS_H
#define SORTIX_GLOBALS_H

#define DECLARE_GLOBAL_OBJECT(Type, Name) extern Type* G##Name;
#define DEFINE_GLOBAL_OBJECT(Type, Name) Type D##Name; Type* G##Name = &D##Name;

#endif


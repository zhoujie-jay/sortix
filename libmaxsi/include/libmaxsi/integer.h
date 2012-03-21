/******************************************************************************

	COPYRIGHT(C) JONAS 'SORTIE' TERMANSEN 2012.

	This file is part of LibMaxsi.

	LibMaxsi is free software: you can redistribute it and/or modify it under
	the terms of the GNU Lesser General Public License as published by the Free
	Software Foundation, either version 3 of the License, or (at your option)
	any later version.

	LibMaxsi is distributed in the hope that it will be useful, but WITHOUT ANY
	WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
	FOR A PARTICULAR PURPOSE. See the GNU Lesser General Public License for
	more details.

	You should have received a copy of the GNU Lesser General Public License
	along with LibMaxsi. If not, see <http://www.gnu.org/licenses/>.

	integer.h
	Functions and utilities related to integers.

******************************************************************************/

#ifndef LIBMAXSI_INTEGER_H
#define LIBMAXSI_INTEGER_H

namespace Maxsi
{
	namespace Error
	{
		template <class T> T DivUp(T a, T b)
		{
			return a / b + (a % b) ? 1 : 0;
		}

		template <class R, class A, class B> R (A a, B b)
		{
			return DivUp<R>((R) a, (R) b);
		}
	}
}

#endif


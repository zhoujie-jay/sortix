/*******************************************************************************

    Copyright(C) Jonas 'Sortie' Termansen 2013.

    This program is free software: you can redistribute it and/or modify it
    under the terms of the GNU General Public License as published by the Free
    Software Foundation, either version 3 of the License, or (at your option)
    any later version.

    This program is distributed in the hope that it will be useful, but WITHOUT
    ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
    FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
    more details.

    You should have received a copy of the GNU General Public License along with
    this program. If not, see <http://www.gnu.org/licenses/>.

    util.h
    Utility functions for the filesystem implementation.

*******************************************************************************/

#ifndef UTIL_H
#define UTIL_H

template <class T> T divup(T a, T b)
{
	return a/b + (a % b ? 1 : 0);
}

template <class T> T roundup(T a, T b)
{
	return a % b ? a + b - a % b : a;
}

inline bool checkbit(const uint8_t* bitmap, size_t bit)
{
	uint8_t bits = bitmap[bit / 8UL];
	return bits & (1U << (bit % 8UL));
}

inline void setbit(uint8_t* bitmap, size_t bit)
{
	bitmap[bit / 8UL] |= 1U << (bit % 8UL);
}

inline void clearbit(uint8_t* bitmap, size_t bit)
{
	bitmap[bit / 8UL] &= ~(1U << (bit % 8UL));
}

#endif

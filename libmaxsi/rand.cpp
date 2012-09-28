/*******************************************************************************

	Copyright(C) Jonas 'Sortie' Termansen 2011, 2012.

	This file is part of LibMaxsi.

	LibMaxsi is free software: you can redistribute it and/or modify it under
	the terms of the GNU Lesser General Public License as published by the Free
	Software Foundation, either version 3 of the License, or (at your option)
	any later version.

	LibMaxsi is distributed in the hope that it will be useful, but WITHOUT ANY
	WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
	FOR A PARTICULAR PURPOSE. See the GNU Lesser General Public License for more
	details.

	You should have received a copy of the GNU Lesser General Public License
	along with LibMaxsi. If not, see <http://www.gnu.org/licenses/>.

	rand.cpp
	Returns a random value.

*******************************************************************************/

#include <stdlib.h>

#if 1

static unsigned int m_w = 1337;
static unsigned int m_z = 37;

static unsigned int RandomUnsignedInt()
{
	m_z = 36969 * (m_z & 65535) + (m_z >> 16);
	m_w = 18000 * (m_w & 65535) + (m_w >> 16);
	return (m_z << 16) + m_w;  /* 32-bit result */
}

extern "C" int rand()
{
	return RandomUnsignedInt() % 32768;
}

#else

static unsigned random_seed = 1337;

extern "C" int rand()
{
	random_seed = random_seed + 37 * 1103515245 + 12345;
	return random_seed >> 16;
}

#endif

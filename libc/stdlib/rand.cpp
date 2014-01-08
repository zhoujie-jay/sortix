/*******************************************************************************

    Copyright(C) Jonas 'Sortie' Termansen 2011, 2012, 2014.

    This file is part of the Sortix C Library.

    The Sortix C Library is free software: you can redistribute it and/or modify
    it under the terms of the GNU Lesser General Public License as published by
    the Free Software Foundation, either version 3 of the License, or (at your
    option) any later version.

    The Sortix C Library is distributed in the hope that it will be useful, but
    WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
    or FITNESS FOR A PARTICULAR PURPOSE. See the GNU Lesser General Public
    License for more details.

    You should have received a copy of the GNU Lesser General Public License
    along with the Sortix C Library. If not, see <http://www.gnu.org/licenses/>.

    stdlib/rand.cpp
    Returns a random value.

*******************************************************************************/

#include <stdint.h>
#include <stdlib.h>

static uint32_t m_w = 1337;
static uint32_t m_z = 37;

static uint32_t random_32_bits()
{
	m_z = 36969 * (m_z >> 0 & 0xFFFF) + (m_z >> 16 & 0xFFFF);
	m_w = 18000 * (m_w >> 0 & 0xFFFF) + (m_w >> 16 & 0xFFFF);
	return (m_z << 16) + m_w;
}

extern "C" int rand()
{
	return (int) (random_32_bits() % ((uint32_t) RAND_MAX + 1));
}

extern "C" void srand(unsigned int seed)
{
	m_w = seed >> 16 & 0xFFFF;
	m_z = seed >>  0 & 0xFFFF;
}

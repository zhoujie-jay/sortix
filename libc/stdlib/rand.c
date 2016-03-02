/*
 * Copyright (c) 2011, 2012, 2014 Jonas 'Sortie' Termansen.
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 *
 * stdlib/rand.c
 * Returns a random value.
 */

#include <pthread.h>
#include <stdint.h>
#include <stdlib.h>

static pthread_mutex_t rand_mutex = PTHREAD_MUTEX_INITIALIZER;
static uint32_t m_w = 1337;
static uint32_t m_z = 37;

static uint32_t random_32_bits(void)
{
	m_z = 36969 * (m_z >> 0 & 0xFFFF) + (m_z >> 16 & 0xFFFF);
	m_w = 18000 * (m_w >> 0 & 0xFFFF) + (m_w >> 16 & 0xFFFF);
	return (m_z << 16) + m_w;
}

int rand(void)
{
	pthread_mutex_lock(&rand_mutex);
	int result = (int) (random_32_bits() % ((uint32_t) RAND_MAX + 1));
	pthread_mutex_unlock(&rand_mutex);
	return result;
}

void srand(unsigned int seed)
{
	pthread_mutex_lock(&rand_mutex);
	m_w = seed >> 16 & 0xFFFF;
	m_z = seed >>  0 & 0xFFFF;
	pthread_mutex_unlock(&rand_mutex);
}

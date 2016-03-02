/*
 * Copyright (c) 2015, 2016 Jonas 'Sortie' Termansen.
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
 * sortix/termios.h
 * Termios types and constants.
 */

#ifndef INCLUDE_SORTIX_TERMIOS_H
#define INCLUDE_SORTIX_TERMIOS_H

#include <sys/cdefs.h>

#define VEOF 0
#define VEOL 1
#define VERASE 2
#define VINTR 3
#define VKILL 4
#define VMIN 5
#define VQUIT 6
#define VSTART 7
#define VSTOP 8
#define VSUSP 9
#define VTIME 10
#define VWERASE 11
#define NCCS 24

#define BRKINT (1U << 0)
#define ICRNL (1U << 1)
#define IGNBRK (1U << 2)
#define IGNCR (1U << 3)
#define IGNPAR (1U << 4)
#define INLCR (1U << 5)
#define INPCK (1U << 6)
#define ISTRIP (1U << 7)
#define IXANY (1U << 8)
#define IXOFF (1U << 9)
#define IXON (1U << 10)
#define PARMRK (1U << 11)

#define OPOST (1U << 0)

#define B0 0
#define B50 50
#define B75 75
#define B110 110
#define B134 134
#define B150 150
#define B200 200
#define B300 300
#define B600 600
#define B1200 1200
#define B1800 1800
#define B2400 2400
#define B4800 4800
#define B9600 9600
#define B19200 19200
#define B38400 38400

#define CS5 (0U << 0)
#define CS6 (1U << 0)
#define CS7 (2U << 0)
#define CS8 (3U << 0)
#define CSIZE (CS5 | CS6 | CS7 | CS8)
#define CSTOPB (1U << 2)
#define CREAD (1U << 3)
#define PARENB (1U << 4)
#define PARODD (1U << 5)
#define HUPCL (1U << 6)
#define CLOCAL (1U << 7)

#define ECHO (1U << 0)
#define ECHOE (1U << 1)
#define ECHOK (1U << 2)
#define ECHONL (1U << 3)
#define ICANON (1U << 4)
#define IEXTEN (1U << 5)
#define ISIG (1U << 6)
#define NOFLSH (1U << 7)
#define TOSTOP (1U << 8)
/* Transitional compatibility as Sortix switches from termmode to termios. */
#if __USE_SORTIX
#define ISORTIX_TERMMODE (1U << 9)
#define ISORTIX_CHARS_DISABLE (1U << 10)
#define ISORTIX_KBKEY (1U << 11)
#define ISORTIX_32BIT (1U << 12)
#define ISORTIX_NONBLOCK (1U << 13)
#endif

#define TCSANOW 0
#define TCSADRAIN 1
#define TCSAFLUSH 2

#define TCIFLUSH (1 << 0)
#define TCOFLUSH (1 << 1)
#define TCIOFLUSH (TCIFLUSH | TCOFLUSH)

#define TCIOFF 0
#define TCION 1
#define TCOOFF 2
#define TCOON 3

typedef unsigned char cc_t;
typedef unsigned int speed_t;
typedef unsigned int tcflag_t;

struct termios
{
	tcflag_t c_iflag;
	tcflag_t c_oflag;
	tcflag_t c_cflag;
	tcflag_t c_lflag;
	speed_t c_ispeed;
	speed_t c_ospeed;
	cc_t c_cc[NCCS];
};

#endif

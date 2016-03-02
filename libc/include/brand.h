/*
 * Copyright (c) 2013, 2014 Jonas 'Sortie' Termansen.
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
 * brand.h
 * Macros expanding to the name of the operating system and distribution.
 */

#ifndef INCLUDE_BRAND_H
#define INCLUDE_BRAND_H

/* The name of the distribution of the operation system. */
#define BRAND_DISTRIBUTION_NAME "Sortix"

/* The website of the distribution. */
#define BRAND_DISTRIBUTION_WEBSITE "https://sortix.org"

/* The name of the operating system. */
#define BRAND_OPERATING_SYSTEM_NAME "Sortix"

/* The name of the kernel. */
#define BRAND_KERNEL_NAME "Sortix"

/* Ascii version of the maxsi logo. */
#define BRAND_MAXSI \
"                                        _  \n" \
"                                       / \\ \n" \
"   /\\    /\\                           /   \\\n" \
"  /  \\  /  \\                          |   |\n" \
" /    \\/    \\                         |   |\n" \
"|  O    O    \\_______________________ /   |\n" \
"|                                         |\n" \
"| \\_______/                               /\n" \
" \\                                       / \n" \
"   ------       ---------------      ---/  \n" \
"        /       \\             /      \\     \n" \
"       /         \\           /        \\    \n" \
"      /           \\         /          \\   \n" \
"     /_____________\\       /____________\\  \n" \
"                                           \n" \

/* Dead version of the maxsi logo, used for panic screens and such. */
#define BRAND_MAXSI_DEAD \
"                                        _  \n" \
"                                       / \\ \n" \
"   /\\    /\\                           /   \\\n" \
"  /  \\  /  \\                          |   |\n" \
" /    \\/    \\                         |   |\n" \
"|  X    X    \\_______________________ /   |\n" \
"|                                         |\n" \
"| _________                               /\n" \
" \\                                       / \n" \
"   ------       ---------------      ---/  \n" \
"        /       \\             /      \\     \n" \
"       /         \\           /        \\    \n" \
"      /           \\         /          \\   \n" \
"     /_____________\\       /____________\\  \n" \
"                                           \n" \

/* Message printed when a critical error occurs and the system panics. */
#define BRAND_PANIC_LONG \
"\e[m\e[31;40m\e[2J\e[H" \
BRAND_MAXSI_DEAD \
"                                                                                \n" \
"                              RED MAXSI OF DEATH                                \n" \
"                                                                                \n" \
"A critical error occured within the kernel of the operating system and it has\n" \
"forcefully shut down as a last resort.\n" \
"\n" \
"Technical information:\n" \

/* Short version of the panic version that consumes minimal space. */
#define BRAND_PANIC_SHORT \
"\e[m\e[31m\e[0J" \
"RED MAXSI OF DEATH\n" \

#endif

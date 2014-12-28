/*******************************************************************************

    Copyright(C) Jonas 'Sortie' Termansen 2013, 2014.

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

    brand.h
    Macros expanding to the name of the operating system and distribution.

*******************************************************************************/

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
"                                                       _                        \n" \
"                                                      / \\                       \n" \
"                  /\\    /\\                           /   \\                      \n" \
"                 /  \\  /  \\                          |   |                      \n" \
"                /    \\/    \\                         |   |                      \n" \
"               |  O    O    \\_______________________ /   |                      \n" \
"               |                                         |                      \n" \
"               | \\_______/                               /                      \n" \
"                \\                                       /                       \n" \
"                  ------       ---------------      ---/                        \n" \
"                       /       \\             /      \\                           \n" \
"                      /         \\           /        \\                          \n" \
"                     /           \\         /          \\                         \n" \
"                    /_____________\\       /____________\\                        \n" \
"                                                                                \n" \

/* Dead version of the maxsi logo, used for panic screens and such. */
#define BRAND_MAXSI_DEAD \
"                                                       _                        \n" \
"                                                      / \\                       \n" \
"                  /\\    /\\                           /   \\                      \n" \
"                 /  \\  /  \\                          |   |                      \n" \
"                /    \\/    \\                         |   |                      \n" \
"               |  X    X    \\_______________________ /   |                      \n" \
"               |                                         |                      \n" \
"               | _________                               /                      \n" \
"                \\                                       /                       \n" \
"                  ------       ---------------      ---/                        \n" \
"                       /       \\             /      \\                           \n" \
"                      /         \\           /        \\                          \n" \
"                     /           \\         /          \\                         \n" \
"                    /_____________\\       /____________\\                        \n" \
"                                                                                \n" \

/* Message printed by the kernel just after boot. */
#define BRAND_KERNEL_BOOT_MESSAGE \
"\e[37;41m\e[2J" \
BRAND_MAXSI \
"                           BOOTING OPERATING SYSTEM...                          "

/* Message printed by init after it takes control. */
#define BRAND_INIT_BOOT_MESSAGE \
"\r\e[m\e[J"

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

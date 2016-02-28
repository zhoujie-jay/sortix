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

    pthread/pthread_keys.c
    pthread_keys global variables.

*******************************************************************************/

#include <pthread.h>
#include <stddef.h>

pthread_mutex_t __pthread_keys_lock = PTHREAD_RECURSIVE_MUTEX_INITIALIZER_NP;
struct pthread_key* __pthread_keys = NULL;
size_t __pthread_keys_used = 0;
size_t __pthread_keys_length = 0;

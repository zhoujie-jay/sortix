/*******************************************************************************

    Copyright(C) Jonas 'Sortie' Termansen 2011, 2012, 2013, 2014.

    This file is part of Sortix.

    Sortix is free software: you can redistribute it and/or modify it under the
    terms of the GNU General Public License as published by the Free Software
    Foundation, either version 3 of the License, or (at your option) any later
    version.

    Sortix is distributed in the hope that it will be useful, but WITHOUT ANY
    WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
    FOR A PARTICULAR PURPOSE. See the GNU General Public License for more
    details.

    You should have received a copy of the GNU General Public License along with
    Sortix. If not, see <http://www.gnu.org/licenses/>.

    sortix/signal.h
    Defines the numeric values for the various supported signals.

*******************************************************************************/

#ifndef SORTIX_INCLUDE_SIGNAL_H
#define SORTIX_INCLUDE_SIGNAL_H

#include <sys/cdefs.h>

__BEGIN_DECLS

#define SIGHUP      1 /* Hangup */
#define SIGINT      2 /* Interrupt */
#define SIGQUIT     3 /* Quit */
#define SIGILL      4 /* Invalid instruction */
#define SIGTRAP     5 /* Trace/breakpoint trap */
#define SIGABRT     6 /* Aborted */
#define SIGBUS      7 /* Bus Error */
#define SIGFPE      8 /* Floating point exception */
#define SIGKILL     9 /* Killed */
#define SIGUSR1     10 /* User defined signal 1 */
#define SIGSEGV     11 /* Segmentation fault */
#define SIGUSR2     12 /* User defined signal 2 */
#define SIGPIPE     13 /* Broken pipe */
#define SIGALRM     14 /* Alarm clock */
#define SIGTERM     15 /* Terminated */
#define SIGSYS      16 /* Bad system call */
#define SIGCHLD     17 /* Child exited */
#define SIGCONT     18 /* Continued */
#define SIGSTOP     19 /* Stopped (signal) */
#define SIGTSTP     20 /* Stopped */
#define SIGTTIN     21 /* Stopped (tty input) */
#define SIGTTOU     22 /* Stopped (tty output) */
#define SIGURG      23 /* Urgent I/O condition */
#define SIGXCPU     24 /* CPU time limit exceeded */
#define SIGXFSZ     25 /* File size limit exceeded */
#define SIGVTALRM   26 /* Virtual timer expired */
#define SIGPWR      27 /* Power Fail/Restart */
#define SIGWINCH    28 /* Window changed */
#define SIGRTMIN    64 /* First user-available real-time signal. */
#define SIGRTMAX    127 /* Last user-available real-time signal. */

#define __SIG_NUM_DECLARED 31
#define __SIG_MAX_NUM 128

#if defined(__is_sortix_kernel)
#define SIG_NUM_DECLARED __SIG_NUM_DECLARED
#define SIG_MAX_NUM __SIG_MAX_NUM
#endif

#define SIG_ERR ((void (*)(int)) -1)
#define SIG_DFL ((void (*)(int)) 0)
#define SIG_IGN ((void (*)(int)) 1)

__END_DECLS

#endif

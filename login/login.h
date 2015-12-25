/*******************************************************************************

    Copyright(C) Jonas 'Sortie' Termansen 2014, 2015.

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

    login.h
    Authenticates users.

*******************************************************************************/

#ifndef LOGIN_H
#define LOGIN_H

struct check
{
	sigset_t oldset;
	struct termios tio;
	pid_t pid;
	int pipe;
	union
	{
		int errnum;
		unsigned char errnum_bytes[sizeof(int)];
	};
	unsigned int errnum_done;
	bool pipe_nonblock;
};

bool login(const char* username);
bool check_real(const char* username, const char* password);
bool check_begin(struct check* chk,
                 const char* username,
                 const char* password,
                 bool restrict_termmode);
bool check_end(struct check* chk, bool* result, bool try);
bool check(const char* username, const char* password);
int graphical(void);
int textual(void);

#endif

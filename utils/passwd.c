/*
 * Copyright (c) 2015 Jonas 'Sortie' Termansen.
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
 * passwd.c
 * Password change.
 */

#include <sys/termmode.h>
#include <sys/types.h>

#include <err.h>
#include <errno.h>
#include <fcntl.h>
#include <pwd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <termios.h>
#include <unistd.h>

static void password(char* buffer,
                     size_t buffer_size,
                     const char* whose,
                     const char* question)
{
	if ( !isatty(0) )
		errx(1, "Input is not a terminal");
	unsigned int termmode;
	gettermmode(0, &termmode);
	settermmode(0, termmode & ~TERMMODE_ECHO);
	if ( whose )
		printf("%s's ", whose);
	printf("%s ", question);
	fflush(stdout);
	fflush(stdin);
	// TODO: This may leave a copy of the password in the stdio buffer.
	fgets(buffer, buffer_size, stdin);
	fflush(stdin);
	printf("\n");
	size_t buffer_length = strlen(buffer);
	if ( buffer_length && buffer[buffer_length-1] == '\n' )
		buffer[--buffer_length] = '\0';
	settermmode(0, termmode);
}

static void compact_arguments(int* argc, char*** argv)
{
	for ( int i = 0; i < *argc; i++ )
	{
		while ( i < *argc && !(*argv)[i] )
		{
			for ( int n = i; n < *argc; n++ )
				(*argv)[n] = (*argv)[n+1];
			(*argc)--;
		}
	}
}

static void version(FILE* fp, const char* argv0)
{
	fprintf(fp, "%s (Sortix) %s\n", argv0, VERSIONSTR);
}

static void help(FILE* fp, const char* argv0)
{
	fprintf(fp, "Usage: %s [OPTION]... [LOGIN]\n", argv0);
}

int main(int argc, char* argv[])
{
	const char* cipher = "blowfish,a";

	const char* argv0 = argv[0];
	for ( int i = 1; i < argc; i++ )
	{
		const char* arg = argv[i];
		if ( arg[0] != '-' || !arg[1] )
			continue;
		argv[i] = NULL;
		if ( !strcmp(arg, "--") )
			break;
		if ( arg[1] != '-' )
		{
			char c;
			while ( (c = *++arg) ) switch ( c )
			{
			case 'c':
				if ( !*(cipher = arg + 1) )
				{
					if ( i + 1 == argc )
					{
						warnx("option requires an argument -- 'c'");
						fprintf(stderr, "Try `%s --help' for more information.\n", argv[0]);
						exit(125);
					}
					cipher = argv[i+1];
					argv[++i] = NULL;
				}
				arg = "c";
				break;
			default:
				fprintf(stderr, "%s: unknown option -- '%c'\n", argv0, c);
				help(stderr, argv0);
				exit(1);
			}
		}
		else if ( !strcmp(arg, "--version") )
			version(stdout, argv0), exit(0);
		else if ( !strcmp(arg, "--help") )
			help(stdout, argv0), exit(0);
		else
		{
			fprintf(stderr, "%s: unrecognized option: %s\n", argv0, arg);
			help(stderr, argv0);
			exit(1);
		}
	}

	compact_arguments(&argc, &argv);

	uid_t my_uid = getuid();
	char* my_username = getlogin();
	if ( !my_username )
		err(1, "failed to get username");
	if ( !(my_username = strdup(my_username)) )
		err(1, "stdup");

	const char* username;
	if ( argc <= 1 )
		username = my_username;
	else if ( argc <= 2 )
		username = argv[1];
	else
		errx(1, "Unexpected extra operand");

	errno = 0;
	struct passwd* pwd = getpwnam(username);
	if ( !pwd && errno == 0 )
		errx(1, "%s: No such user", username);
	if ( !pwd )
		err(1, "%s", username);

	if ( my_uid != 0 && pwd->pw_uid != my_uid )
		errx(1, "You may not change the password for '%s'", username);

	printf("Changing password for %s.\n", username);

	if ( my_uid != 0 )
	{
		char current[128];
		password(current, sizeof(current), pwd->pw_name,
		         "current password (will not echo)");
		if ( crypt_checkpass(current, pwd->pw_passwd) < 0 )
			errx(1, "Wrong password for '%s'", pwd->pw_name);
		explicit_bzero(current, sizeof(current));
	}

	char first[128];
	password(first, sizeof(first), NULL, "Enter new password (will not echo)");
	char second[128];
	password(second, sizeof(second), NULL, "Enter new password (again)");
	if ( strcmp(first, second) != 0 )
		errx(1, "Passwords don't match");
	explicit_bzero(second, sizeof(second));
	char newhash[128];
	if ( crypt_newhash(first, cipher, newhash, sizeof(newhash)) < 0 )
		err(1, "crypt_newhash");
	explicit_bzero(first, sizeof(first));

	// TODO: This is subject to races and is obviously an insecure design.
	//       The backend and coordination of the passwd database should be moved
	//       to its own daemon.
	int fd = open("/etc/passwd.new", O_WRONLY | O_CREAT | O_EXCL, 0644);
	if ( fd < 0 )
		err(1, "/etc/passwd.new");
	fchown(fd, 0, 0); // HACK.
	FILE* fp = fdopen(fd, "w");
	if ( !fp )
		err(1, "fdopen");
	setpwent();
	while ( (errno = 0, pwd = getpwent()) )
	{
		fputs(pwd->pw_name, fp);
		fputc(':', fp);
		if ( !strcmp(pwd->pw_name, username) )
			fputs(newhash, fp);
		else
			fputs(pwd->pw_passwd, fp);
		fputc(':', fp);
		fprintf(fp, "%" PRIuUID, pwd->pw_uid);
		fputc(':', fp);
		fprintf(fp, "%" PRIuGID, pwd->pw_gid);
		fputc(':', fp);
		fputs(pwd->pw_gecos, fp);
		fputc(':', fp);
		fputs(pwd->pw_dir, fp);
		fputc(':', fp);
		fputs(pwd->pw_shell, fp);
		fputc('\n', fp);
		if ( ferror(fp) || fflush(stdout) == EOF )
		{
			unlink("/etc/passwd.new");
			err(1, "/etc/passwd.new");
		}
	}
	if ( errno != 0 )
	{
		unlink("/etc/passwd.new");
		err(1, "getpwent");
	}
	if ( fclose(fp) == EOF )
	{
		unlink("/etc/passwd.new");
		err(1, "/etc/passwd.new");
	}
	if ( rename("/etc/passwd.new", "/etc/passwd") < 0 )
	{
		unlink("/etc/passwd.new");
		err(1, "rename: /etc/passwd.new -> /etc/passwd");
	}

	printf("Changed password for %s.\n", username);

	return 0;
}

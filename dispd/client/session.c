/*
 * Copyright (c) 2012 Jonas 'Sortie' Termansen.
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
 * session.c
 * Handles session management.
 */

#include <sys/types.h>
#include <sys/display.h>
#include <sys/wait.h>

#include <error.h>
#include <fcntl.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <dispd.h>

#include "session.h"

static const uint64_t ONE_AND_ONLY_DEVICE = 0;
static const uint64_t ONE_AND_ONLY_CONNECTOR = 0;

struct dispd_session* global_session = NULL;

bool dispd__session_initialize(int* argc, char*** argv)
{
	(void) argc;
	(void) argv;
	size_t size = sizeof(struct dispd_session);
	global_session = (struct dispd_session*) malloc(size);
	if ( !global_session )
		return false;
	memset(global_session, 0, sizeof(*global_session));
	global_session->device = ONE_AND_ONLY_DEVICE;
	global_session->connector = ONE_AND_ONLY_CONNECTOR;
	return true;
}

struct dispd_session* dispd_attach_default_session()
{
	global_session->refcount++;
	return global_session;
}

bool dispd_detach_session(struct dispd_session* session)
{
	session->refcount--;
	return true;
}

bool dispd_session_setup_game_rgba(struct dispd_session* session)
{
	if ( session->is_rgba )
		return true;
	if ( session->current_window )
		return false;
	struct dispmsg_get_crtc_mode msg;
	msg.msgid = DISPMSG_GET_CRTC_MODE;
	msg.device = session->device;
	msg.connector = session->connector;
	if ( dispmsg_issue(&msg, sizeof(msg)) != 0 )
		return false;
	if ( !(msg.mode.control & 1) || msg.mode.fb_format != 32 )
	{
		pid_t childpid = fork();
		if ( childpid < 0 )
			return false;
		if ( childpid )
		{
			int status;
			waitpid(childpid, &status, 0);
			return session->is_rgba = (WIFEXITED(status) && !WEXITSTATUS(status));
		}
		const char* chvideomode = "chvideomode";
#if 1
		// TODO chvideomode currently launches --bpp 32 as a program...
		execlp(chvideomode, chvideomode, NULL);
#else
		execlp(chvideomode, chvideomode,
		       "--bpp", "32",
		       "--show-graphics", "true",
		       "--show-text", "false",
		       NULL);
#endif
		perror(chvideomode);
		exit(127);
	}

	// HACK: The console may be rendered asynchronously and it might still be
	//       rendering to the framebuffer, however this process is about to do
	//       bitmapped graphics to the framebuffer as well. Since there is no
	//       synchronization with the terminal except not writing to it, there
	//       is a small window where both are fighting for the framebuffer.
	//       We can resolve this issue by simply fsync()ing the terminal, which
	//       causes the scheduled console rendering to finish before returning.
	int tty_fd = open("/dev/tty", O_WRONLY);
	if ( 0 <= tty_fd )
	{
		fsync(tty_fd);
		close(tty_fd);
	}

	return session->is_rgba = true;
}

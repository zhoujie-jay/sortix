/*******************************************************************************

    Copyright(C) Jonas 'Sortie' Termansen 2012.

    This file is part of dispd.

    dispd is free software: you can redistribute it and/or modify it under the
    terms of the GNU Lesser General Public License as published by the Free
    Software Foundation, either version 3 of the License, or (at your option)
    any later version.

    dispd is distributed in the hope that it will be useful, but WITHOUT ANY
    WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
    FOR A PARTICULAR PURPOSE. See the GNU Lesser General Public License for more
    details.

    You should have received a copy of the GNU Lesser General Public License
    along with dispd. If not, see <http://www.gnu.org/licenses/>.

    window.c
    Handles windows.

*******************************************************************************/

#include <sys/types.h>
#include <sys/display.h>

#include <fcntl.h>
#include <ioleast.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <dispd.h>

#include "framebuffer.h"
#include "session.h"
#include "window.h"

struct dispd_window* dispd_create_window_game_rgba(struct dispd_session* session)
{
	if ( session->current_window )
		return NULL;
	if ( !session->is_rgba )
		return NULL;
	size_t size = sizeof(struct dispd_window);
	struct dispd_window* window = (struct dispd_window*) malloc(size);
	if ( !window )
		return NULL;
	memset(window, 0, sizeof(*window));
	window->session = session;
	return session->current_window = window;
}

bool dispd_destroy_window(struct dispd_window* window)
{
	free(window->cached_buffer);
	free(window);
	return true;
}

static uint8_t* request_buffer(struct dispd_window* window, size_t size)
{
	if ( window->cached_buffer )
	{
		if ( window->cached_buffer_size == size )
		{
			uint8_t* ret = window->cached_buffer;
			window->cached_buffer = NULL;
			window->cached_buffer_size = 0;
			return ret;
		}
		free(window->cached_buffer);
		window->cached_buffer = NULL;
		window->cached_buffer_size = 0;
	}
	return (uint8_t*) malloc(size);
}

static void return_buffer(struct dispd_window* window, uint8_t* b, size_t size)
{
	free(window->cached_buffer);
	window->cached_buffer = b;
	window->cached_buffer_size = size;
}

struct dispd_framebuffer* dispd_begin_render(struct dispd_window* window)
{
	struct dispmsg_get_crtc_mode msg;
	msg.msgid = DISPMSG_GET_CRTC_MODE;
	msg.device = window->session->device;
	msg.connector = window->session->connector;
	if ( dispmsg_issue(&msg, sizeof(msg)) != 0 )
		return NULL;
	size_t size = sizeof(struct dispd_framebuffer);
	struct dispd_framebuffer* fb = (struct dispd_framebuffer*) malloc(size);
	if ( !fb )
		return NULL;
	memset(fb, 0, sizeof(*fb));
	fb->window = window;
	fb->width = msg.mode.view_xres;
	fb->height = msg.mode.view_yres;
	fb->bpp = 32;
	fb->pitch = fb->width * fb->bpp / 8;
	fb->datasize = fb->pitch * fb->height;
	fb->data = (uint8_t*) request_buffer(window, fb->datasize);
	if ( !fb->data ) { free(fb); return NULL; }
	return fb;
}

bool dispd_finish_render(struct dispd_framebuffer* fb)
{
	struct dispd_window* window = fb->window;
	bool ret = false;
	struct dispmsg_write_memory msg;
	msg.msgid = DISPMSG_WRITE_MEMORY;
	msg.device = window->session->device;
	msg.offset = 0;
	msg.size = fb->datasize;
	msg.src = fb->data;
	if ( dispmsg_issue(&msg, sizeof(msg)) == 0 )
		ret = true;
	return_buffer(window, fb->data, fb->datasize);
	free(fb);
	return ret;
}

uint8_t* dispd_get_framebuffer_data(struct dispd_framebuffer* fb)
{
	return fb->data;
}

size_t dispd_get_framebuffer_pitch(struct dispd_framebuffer* fb)
{
	return fb->pitch;
}

int dispd_get_framebuffer_format(struct dispd_framebuffer* fb)
{
	return fb->bpp;
}

int dispd_get_framebuffer_height(struct dispd_framebuffer* fb)
{
	return fb->height;
}

int dispd_get_framebuffer_width(struct dispd_framebuffer* fb)
{
	return fb->width;
}

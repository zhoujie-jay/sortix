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

	window.cpp
	Handles windows.

*******************************************************************************/

#include <sys/types.h>
#include <sys/display.h>

#include <fcntl.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <dispd.h>

#include "framebuffer.h"
#include "session.h"
#include "window.h"

extern "C"
struct dispd_window* dispd_create_window_game_vga(struct dispd_session* session)
{
	if ( session->current_window )
		return NULL;
	if ( !session->is_vga )
		return NULL;
	size_t size = sizeof(struct dispd_window);
	struct dispd_window* window = (struct dispd_window*) malloc(size);
	if ( !window )
		return NULL;
	memset(window, 0, sizeof(*window));
	window->session = session;
	window->is_vga = true;
	return session->current_window = window;
}

extern "C"
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
	window->is_rgba = true;
	return session->current_window = window;
}

extern "C" bool dispd_destroy_window(struct dispd_window* window)
{
	free(window);
	return true;
}

static
struct dispd_framebuffer* disp_begin_render_vga(struct dispd_window* window)
{
	size_t size = sizeof(struct dispd_framebuffer);
	struct dispd_framebuffer* fb = (struct dispd_framebuffer*) malloc(size);
	if ( !fb )
		return NULL;
	memset(fb, 0, sizeof(*fb));
	fb->is_vga = true;
	fb->window = window;
	fb->width = 80;
	fb->height = 25;
	fb->bpp = 16;
	fb->pitch = fb->width * fb->bpp / 8;
	fb->datasize = fb->pitch * fb->height;
	fb->data = (uint8_t*) malloc(fb->datasize);
	if ( !fb->data ) { free(fb); return NULL; }
	return fb;
}

static
struct dispd_framebuffer* disp_begin_render_rgba(struct dispd_window* window)
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
	fb->is_rgba = true;
	fb->width = msg.mode.view_xres;
	fb->height = msg.mode.view_yres;
	fb->bpp = 32;
	fb->pitch = fb->width * fb->bpp / 8;
	fb->datasize = fb->pitch * fb->height;
	fb->data = (uint8_t*) malloc(fb->datasize);
	if ( !fb->data ) { free(fb); return NULL; }
	return fb;
}


struct dispd_framebuffer* dispd_begin_render(struct dispd_window* window)
{
	if ( window->is_vga )
		return disp_begin_render_vga(window);
	if ( window->is_rgba )
		return disp_begin_render_rgba(window);
	return NULL;
}

bool dispd_finish_render(struct dispd_framebuffer* fb)
{
	struct dispd_window* window = fb->window;
	bool ret = false;
	if ( window->is_vga )
	{
		int fd = open("/dev/vga", O_WRONLY);
		if ( 0 <= fd )
		{
			if ( writeall(fd, fb->data, fb->datasize) == fb->datasize )
				ret = true;
			close(fd);
		}
	}
	else if ( window->is_rgba )
	{
		struct dispmsg_write_memory msg;
		msg.msgid = DISPMSG_WRITE_MEMORY;
		msg.device = window->session->device;
		msg.offset = 0;
		msg.size = fb->datasize;
		msg.src = fb->data;
		if ( dispmsg_issue(&msg, sizeof(msg)) == 0 )
			ret = true;
	}
	free(fb->data);
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
	return fb->is_vga ? 0 : fb->bpp;
}

int dispd_get_framebuffer_height(struct dispd_framebuffer* fb)
{
	return fb->height;
}

int dispd_get_framebuffer_width(struct dispd_framebuffer* fb)
{
	return fb->width;
}

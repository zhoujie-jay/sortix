/*
 * Copyright (c) 2014, 2015 Jonas 'Sortie' Termansen.
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
 * graphical.c
 * Graphical login.
 */

#include <sys/display.h>
#include <sys/display.h>
#include <sys/kernelinfo.h>
#include <sys/termmode.h>

#include <assert.h>
#include <brand.h>
#include <err.h>
#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <math.h>
#include <poll.h>
#include <signal.h>
#include <sched.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <termios.h>
#include <time.h>
#include <timespec.h>
#include <unistd.h>

// TODO: The Sortix <limits.h> doesn't expose this at the moment.
#if !defined(HOST_NAME_MAX) && defined(__sortix__)
#include <sortix/limits.h>
#endif

#include "framebuffer.h"
#include "login.h"
#include "pixel.h"
#include "vgafont.h"

#include "arrow.inc"

enum stage
{
	STAGE_USERNAME,
	STAGE_PASSWORD,
	STAGE_CHECKING,
};

struct textbox
{
	char text[256];
	size_t used;
	size_t offset;
	const char* standin;
	bool password;
};

static uint32_t arrow_buffer[48 * 48];
static struct framebuffer arrow_framebuffer = { 48, arrow_buffer, 48, 48 };

static inline void arrow_initialize()
{
	static bool done = false;
	if ( done )
		return;
	memcpy(arrow_buffer, arrow, sizeof(arrow));
	done = true;
}

static struct textbox textbox_username;
static struct textbox textbox_password;

struct glogin
{
	struct check chk;
	int fd_mouse;
	struct dispmsg_crtc_mode mode;
	struct framebuffer fade_from_fb;
	struct timespec fade_from_begin;
	struct timespec fade_from_end;
	bool fading_from;
	uint32_t* last_fb_buffer;
	size_t last_fb_buffer_size;
	int pointer_x;
	int pointer_y;
	size_t mouse_byte_count;
	uint8_t mouse_bytes[3];
	enum stage stage;
	bool animating;
	const char* warning;
	bool pointer_working;
};

static struct glogin state;

static bool get_graphical_mode(struct dispmsg_crtc_mode* mode)
{
	struct dispmsg_get_crtc_mode msg;
	memset(&msg, 0, sizeof(msg));
	msg.msgid = DISPMSG_GET_CRTC_MODE;
	msg.device = 0; // TODO: Multi-screen support!
	msg.connector = 0; // TODO: Multi-screen support!
	if ( dispmsg_issue(&msg, sizeof(msg)) != 0 )
	{
		warn("dispmsg_issue: DISPMSG_GET_CRTC_MODE");
		return false;
	}
	*mode = msg.mode;
	return true;
}

static bool is_graphical_mode(struct dispmsg_crtc_mode* mode)
{
	return  (mode->control & DISPMSG_CONTROL_VALID) &&
		   !(mode->control & DISPMSG_CONTROL_VGA) &&
		     mode->fb_format == 32;
}

static void textbox_initialize(struct textbox* textbox, const char* standin)
{
	memset(textbox, 0, sizeof(*textbox));
	textbox->standin = standin;
}

static void textbox_reset(struct textbox* textbox)
{
	explicit_bzero(textbox->text, sizeof(textbox->text));
	textbox->used = 0;
	textbox->offset = 0;
}

static void textbox_type_char(struct textbox* textbox, char c)
{
	if ( textbox->used + 1 == sizeof(textbox->text) )
		return;
	memmove(textbox->text + textbox->offset + 1,
	        textbox->text + textbox->offset,
	        textbox->used - textbox->offset + 1);
	textbox->text[textbox->offset++] = c;
	textbox->used++;
}

static void textbox_type_backspace(struct textbox* textbox)
{
	if ( textbox->offset == 0 )
		return;
	memmove(textbox->text + textbox->offset - 1,
	        textbox->text + textbox->offset,
	        textbox->used - textbox->offset + 1);
	textbox->offset--;
	textbox->used--;
}

void render_right_text(struct framebuffer fb, const char* str, uint32_t color)
{
	size_t len = strlen(str);
	for ( size_t i = 0; i < len; i++ )
	{
		int x = fb.xres - ((int) FONT_WIDTH+1) * ((int) len - (int) i);
		render_char(framebuffer_crop(fb, x, 0, fb.xres, fb.yres), str[i], color);
	}
}

void render_right_text_if_needed(struct framebuffer fb, const char* str, uint32_t color)
{
	size_t len = strlen(str);
	size_t shown_len = fb.xres / (FONT_WIDTH+1);
	if ( len <= shown_len )
		render_text(fb, str, color);
	else
		render_right_text(fb, str, color);
}

static void render_background(struct framebuffer fb)
{
	uint32_t bg_color = make_color(0x89 * 2/3, 0xc7 * 2/3, 0xff * 2/3);
	for ( size_t y = 0; y < fb.yres; y++ )
		for ( size_t x = 0; x < fb.xres; x++ )
			framebuffer_set_pixel(fb, x, y, bg_color);
}

static void render_pointer(struct framebuffer fb)
{
	int p_hwidth = arrow_framebuffer.xres / 2;
	int p_hheight = arrow_framebuffer.yres / 2;
	int p_x = state.pointer_x - p_hwidth;
	int p_y = state.pointer_y - p_hheight;
	struct framebuffer arrow_render = arrow_framebuffer;
	if ( p_x < 0 )
	{
		arrow_render = framebuffer_crop(arrow_render, -p_x, 0, arrow_render.xres, arrow_render.yres);
		p_x = 0;
	}
	if ( p_y < 0 )
	{
		arrow_render = framebuffer_crop(arrow_render, 0, -p_y, arrow_render.xres, arrow_render.yres);
		p_y = 0;
	}
	struct framebuffer fb_dst = framebuffer_crop(fb, p_x, p_y, fb.xres, fb.yres);
	framebuffer_copy_to_framebuffer_blend(fb_dst, arrow_render);
}

static char* brand_line()
{
	char version[64];
	version[0] = '\0';
	kernelinfo("version", version, sizeof(version));
	char* result = NULL;
	asprintf(&result, "%s %s - %s",
	         BRAND_OPERATING_SYSTEM_NAME,
	         version,
	         BRAND_DISTRIBUTION_WEBSITE);
	return result;
}

static void render_information(struct framebuffer fb)
{
	struct framebuffer textfb;

	char* brandstr = brand_line();
	if ( brandstr )
	{
		textfb = fb;
		textfb = framebuffer_center_text_x(textfb, fb.xres/2, brandstr);
		textfb = framebuffer_bottom_text_y(textfb, fb.yres, brandstr);
		render_text(textfb, brandstr, make_color(255, 255, 255));
		free(brandstr);
	}
}

static void render_textbox(struct framebuffer fb, struct textbox* textbox)
{
	for ( int y = 0; y < (int) fb.yres; y++ )
	{
		for ( int x = 0; x < (int) fb.xres; x++ )
		{
			uint32_t color;
			if ( x == 0 || x == (int) fb.xres - 1 ||
			     y == 0 || y == (int) fb.yres - 1 )
				color = make_color(32, 32, 32);
			else
				color = make_color(255, 255, 255);
			framebuffer_set_pixel(fb, x, y, color);
		}
	}

	fb = framebuffer_cut_left_x(fb, 6);
	fb = framebuffer_cut_right_x(fb, 6);
	fb = framebuffer_cut_top_y(fb, 6);
	fb = framebuffer_cut_bottom_y(fb, 6);
	if ( !textbox->used )
		render_right_text_if_needed(fb, textbox->standin, make_color(160, 160, 160));
	else if ( textbox->password )
	{
		int x = 0;
		while ( x + (FONT_WIDTH+1) <= (int) fb.xres )
		{
			render_char(framebuffer_crop(fb, x, 0, fb.xres, fb.yres), '*',
			            make_color(200, 200, 200));
			x += (FONT_WIDTH+1);
		}
	}
	else
		render_right_text_if_needed(fb, textbox->text, make_color(0, 0, 0));
}

static void render_form(struct framebuffer fb)
{
	int typearea_width = (FONT_WIDTH + 1) * 25;
	int typearea_height = FONT_HEIGHT;
	int textbox_margin = 6;
	int textbox_width = typearea_width + 2 * textbox_margin;
	int textbox_height = typearea_height + 2 * textbox_margin;
	int form_margin = 10;
	int form_width = textbox_width + 2 * form_margin;
	int form_height = textbox_height + 2 * form_margin;
	int BORDER_WIDTH = 8;
	int TITLE_HEIGHT = 28;
	int b0 = 0;
	int b1 = 1;
	int b2 = 2;
	int b3 = BORDER_WIDTH;
	int t0 = TITLE_HEIGHT;
	int window_width = BORDER_WIDTH + form_width + BORDER_WIDTH;
	int window_height = TITLE_HEIGHT + form_height + BORDER_WIDTH;

	if ( state.warning )
	{
		struct framebuffer warnfb = fb;
		int y = (fb.yres - 50 - window_height) / 2 - 2 * FONT_HEIGHT;
		warnfb = framebuffer_cut_top_y(warnfb, y);
		int w = strlen(state.warning) * (FONT_WIDTH+1);
		warnfb = framebuffer_center_x(warnfb, fb.xres / 2, w);
		render_text(warnfb, state.warning, make_color(255, 0, 0));
	}

	fb = framebuffer_center_x(fb, fb.xres / 2, window_width);
	fb = framebuffer_center_y(fb, (fb.yres - 50) / 2, window_height);

	uint32_t glass_color = make_color_a(200, 200, 255, 192);
	uint32_t title_color = make_color_a(16, 16, 16, 240);

	for ( int y = 0; y < (int) fb.yres; y++ )
	{
		for ( int x = 0; x < (int) fb.xres; x++ )
		{
			uint32_t color;
			if ( x == b0 || x == (int) fb.xres - (b0+1) ||
			     y == b0 || y == (int) fb.yres - (b0+1) )
				color = make_color_a(0, 0, 0, 32);
			else if ( x == b1 || x == (int) fb.xres - (b1+1) ||
			          y == b1 || y == (int) fb.yres - (b1+1) )
				color = make_color_a(0, 0, 0, 64);
			else if ( x == b2 || x == (int) fb.xres - (b2+1) ||
			          y == b2 || y == (int) fb.yres - (b2+1) )
				color = make_color(240, 240, 250);
			else if ( x < (b3-1) || x > (int) fb.xres - (b3+1-1) ||
			          y < (t0-1) || y > (int) fb.yres - (b3+1-1) )
				color = glass_color;
			else if ( x == (b3-1) || x == (int) fb.xres - (b3+1-1) ||
			          y == (t0-1) || y == (int) fb.yres - (b3+1-1) )
				color = make_color(64, 64, 64);
			else
				continue;
			uint32_t bg = framebuffer_get_pixel(fb, x, y);
			framebuffer_set_pixel(fb, x, y, blend_pixel(bg, color));
		}
	}

	fb = framebuffer_cut_left_x(fb, BORDER_WIDTH);
	fb = framebuffer_cut_right_x(fb, BORDER_WIDTH);

	char hostname[HOST_NAME_MAX + 1];
	hostname[0] = '\0';
	gethostname(hostname, sizeof(hostname));
	const char* tt = hostname;
	size_t tt_length = strlen(tt);
	size_t tt_max_width = fb.xres;
	size_t tt_desired_width = tt_length * (FONT_WIDTH+1);
	size_t tt_width = tt_desired_width < tt_max_width ? tt_desired_width : tt_max_width;
	size_t tt_height = FONT_HEIGHT;
	size_t tt_pos_x = BORDER_WIDTH + (tt_max_width - tt_width) / 2;
	size_t tt_pos_y = (TITLE_HEIGHT - FONT_HEIGHT) / 2 + 2;
	uint32_t tt_color = title_color;
	render_text(framebuffer_crop(fb, tt_pos_x, tt_pos_y, tt_width, tt_height), tt, tt_color);

	fb = framebuffer_cut_top_y(fb, TITLE_HEIGHT);
	fb = framebuffer_cut_bottom_y(fb, BORDER_WIDTH);

	for ( int y = 0; y < (int) fb.yres; y++ )
	{
		for ( int x = 0; x < (int) fb.xres; x++ )
		{
			framebuffer_set_pixel(fb, x, y, make_color(214, 214, 214));
		}
	}

	struct framebuffer boxfb = fb;
	boxfb = framebuffer_cut_left_x(boxfb, form_margin);
	boxfb = framebuffer_cut_right_x(boxfb, form_margin);
	boxfb = framebuffer_cut_top_y(boxfb, form_margin);
	boxfb = framebuffer_cut_bottom_y(boxfb, form_margin);
	switch ( state.stage )
	{
	case STAGE_USERNAME: render_textbox(boxfb, &textbox_username); break;
	case STAGE_PASSWORD: render_textbox(boxfb, &textbox_password); break;
	default: break;
	}
}

static void render_progress(struct framebuffer fb)
{
	state.animating = true;
	struct timespec now;
	clock_gettime(CLOCK_MONOTONIC, &now);
	float time = (float) now.tv_sec + (float) now.tv_nsec * 10E-9;
	float rotslow_cos = cos(-time / 30.0f * M_PI * 2.0);
	float rotslow_sin = sin(-time / 30.0f * M_PI * 2.0);
	int size = 32;
	int width = 4;
	float widthf = ((float) width / (float) size) * 2.0f;
	float innersq = (1.0f - widthf) * (1.0f - widthf);
	float outersq = (1.0f) * (1.0f);
	fb = framebuffer_center_x(fb, fb.xres / 2, size);
	fb = framebuffer_center_y(fb, (fb.yres - 50) / 2, size);
	for ( size_t y = 0; y < fb.yres; y++ )
	{
		float yfi = ((float) y / (float) size) * 2.0f - 1.0f;
		for ( size_t x = 0; x < fb.xres; x++ )
		{
			float xfi = ((float) x / (float) size) * 2.0f - 1.0f;
			float distsq = xfi * xfi + yfi * yfi;
			if ( distsq < innersq )
				continue;
			if ( distsq > outersq )
				continue;
			float af = fabs((distsq - innersq) / (outersq - innersq) * 2.0f - 1.0f);
			af = 1.0 - af * af;
			uint8_t a = (uint8_t) (af * 255.0f);
			float xf = xfi;
			float yf = yfi;
			xf =  rotslow_cos * xf + rotslow_sin * yf;
			yf = -rotslow_sin * xf + rotslow_cos * yf;
			if ( -widthf < yf && yf < widthf )
				continue;
			uint8_t r = 0;
			uint8_t g = 127.5 + 127.5 * xf;
			uint8_t b = 255;
			uint32_t bg = framebuffer_get_pixel(fb, x, y);
			uint32_t fg = make_color_a(r, g, b, a);
			framebuffer_set_pixel(fb, x, y, blend_pixel(bg, fg));
		}
	}
}

static void render_login(struct framebuffer fb)
{
	render_background(fb);
	if ( false )
		render_information(fb);
	switch ( state.stage )
	{
	case STAGE_USERNAME: render_form(fb); break;
	case STAGE_PASSWORD: render_form(fb); break;
	case STAGE_CHECKING: render_progress(fb); break;
	}
	if ( state.pointer_working )
		render_pointer(fb);
}

static void glogin_fade_from_end(struct glogin* state)
{
	state->fading_from = false;
	free(state->fade_from_fb.buffer);
}

static uint32_t* glogin_malloc_fb_buffer(struct glogin* state,
                                         size_t size)
{
	if ( state->last_fb_buffer )
	{
		if ( state->last_fb_buffer_size == size )
		{
			uint32_t* result = state->last_fb_buffer;
			state->last_fb_buffer = NULL;
			return result;
		}
		free(state->last_fb_buffer);
		state->last_fb_buffer = NULL;
	}
	uint32_t* result = (uint32_t*) malloc(size);
	if ( !result )
	{
		glogin_fade_from_end(state);
		result = (uint32_t*) malloc(size);
	}
	return result;
}

static void glogin_free_fb_buffer(struct glogin* state,
                                  uint32_t* buffer,
                                  size_t size)
{
	if ( state->last_fb_buffer )
		free(state->last_fb_buffer);
	state->last_fb_buffer = buffer;
	state->last_fb_buffer_size = size;
}

static bool screen_capture(struct glogin* state, struct framebuffer* fb)
{
	fb->xres = state->mode.view_xres;
	fb->yres = state->mode.view_yres;
	fb->pitch = state->mode.view_xres;
	size_t size = sizeof(uint32_t) * fb->xres * fb->yres;
	fb->buffer = (uint32_t*) glogin_malloc_fb_buffer(state, size);
	if ( !fb->buffer )
		return false;
	struct dispmsg_write_memory msg;
	memset(&msg, 0, sizeof(msg));
	msg.msgid = DISPMSG_READ_MEMORY;
	msg.device = 0; // TODO: Multi-screen support!
	msg.offset = 0; // TODO: mode.fb_location!
	msg.size = fb->xres * fb->yres * sizeof(fb->buffer[0]);
	msg.src = (uint8_t*) fb->buffer;
	if ( dispmsg_issue(&msg, sizeof(msg)) != 0 )
	{
		warn("dispmsg_issue: DISPMSG_READ_MEMORY");
		return false;
	}
	return true;
}

static bool begin_render(struct glogin* state, struct framebuffer* fb)
{
	if ( !get_graphical_mode(&state->mode) )
		return false;
	fb->xres = state->mode.view_xres;
	fb->yres = state->mode.view_yres;
	fb->pitch = state->mode.view_xres;
	size_t size = sizeof(uint32_t) * fb->xres * fb->yres;
	fb->buffer = (uint32_t*) glogin_malloc_fb_buffer(state, size);
	if ( !fb->buffer )
	{
		warn("malloc");
		return false;
	}
	return true;
}

static bool finish_render(struct glogin* state, struct framebuffer* fb)
{
	struct dispmsg_write_memory msg;
	memset(&msg, 0, sizeof(msg));
	msg.msgid = DISPMSG_WRITE_MEMORY;
	msg.device = 0; // TODO: Multi-screen support!
	msg.offset = 0; // TODO: mode.fb_location!
	msg.size = sizeof(uint32_t) * fb->xres * fb->yres;
	msg.src = (uint8_t*) fb->buffer;
	if ( dispmsg_issue(&msg, sizeof(msg)) != 0 )
	{
		warn("dispmsg_issue: DISPMSG_WRITE_MEMORY");
		free(fb->buffer);
		return false;
	}
	glogin_free_fb_buffer(state, fb->buffer, msg.size);
	return true;
}

static bool render(struct glogin* state)
{
	state->animating = false;
	struct framebuffer fb;
	if ( !begin_render(state, &fb) )
		return false;
	render_login(fb);
	struct timespec now;
	clock_gettime(CLOCK_MONOTONIC, &now);
	if ( state->fading_from && timespec_lt(now, state->fade_from_end) )
	{
		struct timespec duration_ts =
			timespec_sub(state->fade_from_end, state->fade_from_begin);
		struct timespec elapsed_ts = timespec_sub(now, state->fade_from_begin);
		float duration = (float) duration_ts.tv_sec + (float) duration_ts.tv_nsec * 10E-9;
		float elapsed = (float) elapsed_ts.tv_sec + (float) elapsed_ts.tv_nsec * 10E-9;
		float fade_from_alpha_f = 255.0 * elapsed / duration;
		if ( fade_from_alpha_f < 0.0f ) fade_from_alpha_f = 0.0f;
		if ( fade_from_alpha_f > 255.0f ) fade_from_alpha_f = 255.0f;
		uint8_t fade_from_alpha = (uint8_t) fade_from_alpha_f;
		uint32_t and_mask = ~make_color(0, 0, 0);
		uint32_t or_mask = make_color_a(0, 0, 0, 255 - fade_from_alpha);
		for ( int y = 0; y < (int) state->fade_from_fb.yres; y++ )
		{
			for ( int x = 0; x < (int) state->fade_from_fb.xres; x++ )
			{
				uint32_t color = framebuffer_get_pixel(state->fade_from_fb, x, y);
				color = (color & and_mask) | or_mask;
				framebuffer_set_pixel(state->fade_from_fb, x, y, color);
			}
		}
		framebuffer_copy_to_framebuffer_blend(fb, state->fade_from_fb);
		state->animating = true;
	}

	else if ( state->fading_from )
		glogin_fade_from_end(state);
	if ( !finish_render(state, &fb) )
		return false;
	return true;
}

static void think(struct glogin* state)
{
	if ( state->stage == STAGE_CHECKING )
	{
		bool result;
		if ( !check_end(&state->chk, &result, true) )
		{
			sched_yield();
			return;
		}
		if ( result )
		{
			if ( !login(textbox_username.text) )
				state->warning = strerror(errno);
			state->stage = STAGE_USERNAME;
			textbox_reset(&textbox_username);
		}
		else
		{
			state->stage = STAGE_USERNAME;
			textbox_reset(&textbox_username);
			if ( errno == EACCES )
				state->warning = "Invalid username/password";
			else
				state->warning = strerror(errno);
		}
	}
}

static void keyboard_event(struct glogin* state, uint32_t codepoint)
{
	if ( codepoint == '\n' )
	{
		state->warning = NULL;
		switch ( state->stage )
		{
		case STAGE_USERNAME:
			if ( !strcmp(textbox_username.text, "exit") )
				exit(0);
			if ( !strcmp(textbox_username.text, "poweroff") )
				exit(0);
			if ( !strcmp(textbox_username.text, "reboot") )
				exit(1);
			state->stage = STAGE_PASSWORD;
			textbox_reset(&textbox_password);
			break;
		case STAGE_PASSWORD:
			state->stage = STAGE_CHECKING;
			if ( !check_begin(&state->chk, textbox_username.text,
			                 textbox_password.text, true) )
			{
				state->stage = STAGE_USERNAME;
				state->warning = strerror(errno);
			}
			break;
		case STAGE_CHECKING:
			break;
		}
		return;
	}
	struct textbox* textbox = NULL;
	switch ( state->stage )
	{
	case STAGE_USERNAME: textbox = &textbox_username; break;
	case STAGE_PASSWORD: textbox = &textbox_password; break;
	case STAGE_CHECKING: break;
	}
	if ( textbox && codepoint < 128 )
	{
		if ( codepoint == '\b' || codepoint == 127 )
			textbox_type_backspace(textbox);
		else
			textbox_type_char(textbox, (char) codepoint);
	}
}

static void mouse_event(struct glogin* state, unsigned char byte)
{
	state->pointer_working = true;
	if ( state->mouse_byte_count == 0 && !(byte & 1 << 3) )
		return;
	if ( state->mouse_byte_count < 3 )
		state->mouse_bytes[state->mouse_byte_count++] = byte;
	if ( state->mouse_byte_count < 3 )
		return;
	state->mouse_byte_count = 0;
	unsigned char* bytes = state->mouse_bytes;
	int xm = bytes[1];
	int ym = bytes[2];
	if ( xm && bytes[0] & (1 << 4) )
		xm = xm - 256;
	if ( ym && bytes[0] & (1 << 5) )
		ym = ym - 256;
	if ( (bytes[0] & (1 << 6)) || (bytes[0] & (1 << 7)) )
	{
		xm = 0;
		ym = 0;
	}
	ym = -ym;
	int old_pointer_x = state->pointer_x;
	int old_pointer_y = state->pointer_y;
	if ( xm*xm + ym*ym >= 2*2 )
	{
		xm *= 2;
		ym *= 2;
	}
	else if ( xm*xm + ym*ym >= 5*5 )
	{
		xm *= 3;
		ym *= 3;
	}
	state->pointer_x += xm;
	state->pointer_y += ym;
	if ( state->pointer_x < 0 )
		state->pointer_x = 0;
	if ( state->pointer_y < 0 )
		state->pointer_y = 0;
	if ( state->mode.view_xres <= (size_t) state->pointer_x )
		state->pointer_x = state->mode.view_xres;
	if ( state->mode.view_yres <= (size_t) state->pointer_y )
		state->pointer_y = state->mode.view_yres;
	xm = state->pointer_x - old_pointer_x;
	ym = state->pointer_y - old_pointer_y;
	if ( (bytes[0] & 1 << 0) )
	{
		(void) xm;
		(void) ym;
	}
}

void glogin_destroy(struct glogin* state)
{
	if ( 0 <= state->fd_mouse )
		close(state->fd_mouse);
	if ( state->fading_from )
		free(state->fade_from_fb.buffer);
}

bool glogin_init(struct glogin* state)
{
	memset(state, 0, sizeof(*state));
	state->fd_mouse = -1;

	if ( !get_graphical_mode(&state->mode) )
	{
		warn("dispmsg_issue");
		glogin_destroy(state);
		return false;
	}
	if ( !is_graphical_mode(&state->mode) ||
	     state->mode.view_xres < 128 ||
	     state->mode.view_yres < 128 )
	{
		glogin_destroy(state);
		return false;
	}
	if ( !load_font() )
	{
		warn("/dev/vgafont");
		glogin_destroy(state);
		return false;
	}
    state->fd_mouse = open("/dev/mouse", O_RDONLY | O_CLOEXEC);
	if ( settermmode(0, TERMMODE_KBKEY | TERMMODE_UNICODE | TERMMODE_NONBLOCK) < 0 )
	{
		warn("settermmode");
		return false;
	}
	fsync(0);
	arrow_initialize();
	textbox_initialize(&textbox_username, "Username");
	textbox_initialize(&textbox_password, "Password");
	textbox_password.password = true;
	state->pointer_x = state->mode.view_xres / 2;
	state->pointer_y = state->mode.view_yres / 2;
	if ( screen_capture(state, &state->fade_from_fb) )
	{
		state->fading_from = true;
		clock_gettime(CLOCK_MONOTONIC, &state->fade_from_begin);
		struct timespec duration = timespec_make(0, 150*1000*1000);
		state->fade_from_end = timespec_add(state->fade_from_begin, duration);
	}
	return true;
}

int glogin_main(struct glogin* state)
{
	while ( true )
	{
		think(state);
		if ( !render(state) )
			break;
		struct pollfd pfds[2];
		memset(pfds, 0, sizeof(pfds));
		pfds[0].fd = -1;
		pfds[1].fd = -1;
		if ( state->stage != STAGE_CHECKING )
		{
			pfds[0].fd = 0;
			pfds[0].events = POLLIN;
			pfds[0].revents = 0;
		}
		if ( 0 <= state->fd_mouse )
		{
			pfds[1].fd = state->fd_mouse;
			pfds[1].events = POLLIN;
			pfds[1].revents = 0;
		}
		nfds_t nfds = 2;
		struct timespec wake_now_ts = timespec_make(0, 0);
		struct timespec* wake = state->animating ? &wake_now_ts : NULL;
		int num_events = ppoll(pfds, nfds, wake, NULL);
		if ( num_events < 0 )
		{
			warn("poll");
			break;
		}
		for ( nfds_t i = 0; i < nfds; i++ )
		{
			if ( pfds[i].fd == -1 )
				continue;
			if ( (pfds[i].revents & POLLERR) ||
			     (pfds[i].revents & POLLHUP) ||
			     (pfds[i].revents & POLLNVAL) )
			{
				warnx("poll failure on %s", i == 0 ? "keyboard" : "mouse");
				break;
			}
		}
		if ( pfds[0].fd != -1 && pfds[0].revents )
		{
			uint32_t codepoint;
			while ( read(0, &codepoint, sizeof(codepoint)) == sizeof(codepoint) )
				keyboard_event(state, codepoint);
		}
		if ( pfds[1].fd != -1 && pfds[1].revents )
		{
			unsigned char events[64];
			ssize_t amount = read(state->fd_mouse, events, sizeof(events));
			for ( ssize_t i = 0; i < amount; i++ )
				mouse_event(state, events[i]);
		}
	}
	return -1;
}

int graphical(void)
{
	if ( access("/etc/login.conf.textual", F_OK) == 0 )
		return -1;
	if ( !glogin_init(&state) )
		return -1;
	int result = glogin_main(&state);
	glogin_destroy(&state);
	return result;
}

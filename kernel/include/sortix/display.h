/*
 * Copyright (c) 2012, 2014 Jonas 'Sortie' Termansen.
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
 * sortix/display.h
 * Display message declarations.
 */

#ifndef SORTIX_INCLUDE_DISPLAY_H
#define SORTIX_INCLUDE_DISPLAY_H

#include <sys/cdefs.h>

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

static const uint32_t DISPMSG_CONTROL_VALID = 1 << 0;
static const uint32_t DISPMSG_CONTROL_VGA = 1 << 1;
static const uint32_t DISPMSG_CONTROL_OTHER_RESOLUTIONS = 1 << 2;

struct dispmsg_string
{
	size_t byte_size; // Including the terminating NUL-byte.
	char* str;
};

struct dispmsg_crtc_mode
{
	uint64_t driver_index;
	uint64_t magic;
	uint32_t control;
	uint32_t fb_format;
	uint32_t view_xres;
	uint32_t view_yres;
	uint64_t fb_location;
	uint64_t pitch;
	uint32_t surf_off_x;
	uint32_t surf_off_y;
	uint32_t start_x;
	uint32_t start_y;
	uint32_t end_x;
	uint32_t end_y;
	uint32_t desktop_height;
};

struct dispmsg_header
{
	uint64_t msgid; // in
};

#define DISPMSG_ENUMERATE_DEVICES 0x00
struct dispmsg_enumerate_devices
{
	uint64_t msgid; // in
	size_t devices_length; // in, out
	uint64_t* devices; // in, *out
};

#define DISPMSG_GET_DRIVER_COUNT 0x01
struct dispmsg_get_driver_count
{
	uint64_t msgid; // in
	uint64_t device; // in
	uint64_t driver_count; // out
};

#define DISPMSG_GET_DRIVER_NAME 0x02
struct dispmsg_get_driver_name
{
	uint64_t msgid; // in
	uint64_t device; // in
	uint64_t driver_index; // in
	struct dispmsg_string name; // out
};

#define DISPMSG_GET_DRIVER 0x03
struct dispmsg_get_driver
{
	uint64_t msgid; // in
	uint64_t device; // in
	uint64_t driver_index; // out
};

#define DISPMSG_SET_DRIVER 0x04
struct dispmsg_set_driver
{
	uint64_t msgid; // in
	uint64_t device; // in
	uint64_t driver_index; // in
};

#define DISPMSG_SET_CRTC_MODE 0x05
struct dispmsg_set_crtc_mode
{
	uint64_t msgid; // in
	uint64_t device; // in
	uint64_t connector; // in
	struct dispmsg_crtc_mode mode; // in
};

#define DISPMSG_GET_CRTC_MODE 0x06
struct dispmsg_get_crtc_mode
{
	uint64_t msgid; // in
	uint64_t device; // in
	uint64_t connector; // in
	struct dispmsg_crtc_mode mode; // out
};

#define DISPMSG_GET_CRTC_MODES 0x07
struct dispmsg_get_crtc_modes
{
	uint64_t msgid; // in
	uint64_t device; // in
	uint64_t connector; // in
	size_t modes_length; // in, out
	struct dispmsg_crtc_mode* modes; // in, *out
};

#define DISPMSG_GET_MEMORY_SIZE 0x08
struct dispmsg_get_memory_size
{
	uint64_t msgid; // in
	uint64_t device; // in
	uint64_t memory_size; // out
};

#define DISPMSG_WRITE_MEMORY 0x09
struct dispmsg_write_memory
{
	uint64_t msgid; // in
	uint64_t device; // in
	uint64_t offset; // in
	size_t size; // in
	uint8_t* src; // in, *in
};

#define DISPMSG_READ_MEMORY 0x0A
struct dispmsg_read_memory
{
	uint64_t msgid; // in
	uint64_t device; // in
	uint64_t offset; // in
	size_t size; // in
	uint8_t* dst; // in, *out
};

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif

/*
 * Copyright (c) 2011, 2012, 2013, 2014 Jonas 'Sortie' Termansen.
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
 * sortix/kernel/pipe.h
 * Embeddedable one-way data stream.
 */

#ifndef INCLUDE_SORTIX_KERNEL_PIPE_H
#define INCLUDE_SORTIX_KERNEL_PIPE_H

#include <sys/types.h>

#include <sortix/kernel/ioctx.h>
#include <sortix/kernel/poll.h>

namespace Sortix {

class PipeChannel;

class PipeEndpoint
{
public:
	PipeEndpoint();
	~PipeEndpoint();
	bool Connect(PipeEndpoint* destination);
	void Disconnect();
	bool GetSIGPIPEDelivery();
	bool SetSIGPIPEDelivery(bool deliver_sigpipe);
	size_t Size();
	bool Resize(size_t new_size);
	ssize_t read(ioctx_t* ctx, uint8_t* buf, size_t count);
	ssize_t write(ioctx_t* ctx, const uint8_t* buf, size_t count);
	int poll(ioctx_t* ctx, PollNode* node);

private:
	PipeChannel* channel;
	bool reading;

};

} // namespace Sortix

#endif

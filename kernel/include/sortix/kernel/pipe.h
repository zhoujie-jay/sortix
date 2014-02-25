/*******************************************************************************

    Copyright(C) Jonas 'Sortie' Termansen 2011, 2012, 2013.

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

    sortix/kernel/pipe.h
    Embeddedable one-way data stream.

*******************************************************************************/

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
	ssize_t read(ioctx_t* ctx, uint8_t* buf, size_t count);
	ssize_t write(ioctx_t* ctx, const uint8_t* buf, size_t count);
	int poll(ioctx_t* ctx, PollNode* node);

private:
	PipeChannel* channel;
	bool reading;

};

} // namespace Sortix

#endif

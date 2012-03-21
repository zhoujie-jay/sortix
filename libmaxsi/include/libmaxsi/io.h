/******************************************************************************

	COPYRIGHT(C) JONAS 'SORTIE' TERMANSEN 2011.

	This file is part of LibMaxsi.

	LibMaxsi is free software: you can redistribute it and/or modify it under
	the terms of the GNU Lesser General Public License as published by the Free
	Software Foundation, either version 3 of the License, or (at your option)
	any later version.

	LibMaxsi is distributed in the hope that it will be useful, but WITHOUT ANY
	WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
	FOR A PARTICULAR PURPOSE. See the GNU Lesser General Public License for
	more details.

	You should have received a copy of the GNU Lesser General Public License
	along with LibMaxsi. If not, see <http://www.gnu.org/licenses/>.

	io.h
	Functions for management of input and output.

******************************************************************************/

#ifndef LIBMAXSI_IO_H
#define LIBMAXSI_IO_H

namespace Maxsi
{
	namespace IO
	{
		// TODO:

		enum Seek_t { SEEK_SET, SEEK_CUR, SEEK_END };

		int Open(const char* Path, int Flags, int Mode = 0); // TODO: Better default value for Mode
		int Close(int FileDesc = -1);
		int Truncate(int FileDesc, intmax_t Size = 0);
		int Sync(int FileDesc = -1);
		intmax_t Seek(int FileDesc, intmax_t Offset, Seek_t Whence = SEEK_SET);
		size_t Read(int FileDesc, const void* Buffer, size_t BufferSize);
		size_t Write(int FileDesc, const void* Buffer, size_t BufferSize);
		size_t ReadAt(int FileDesc, const void* Buffer, size_t BufferSize, intmax_t Position);
		size_t WriteAt(int FileDesc, const void* Buffer, size_t BufferSize, intmax_t Position);
	}

	size_t Print(const char* msg);
	size_t PrintF(const char* format, ...);
}

#endif


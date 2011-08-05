/******************************************************************************

	COPYRIGHT(C) JONAS 'SORTIE' TERMANSEN 2011.

	This file is part of Sortix.

	Sortix is free software: you can redistribute it and/or modify it under the
	terms of the GNU General Public License as published by the Free Software
	Foundation, either version 3 of the License, or (at your option) any later
	version.

	Sortix is distributed in the hope that it will be useful, but WITHOUT ANY
	WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
	FOR A PARTICULAR PURPOSE. See the GNU General Public License for more
	details.

	You should have received a copy of the GNU General Public License along
	with Sortix. If not, see <http://www.gnu.org/licenses/>.

	mxfs.h
	A working file system.

******************************************************************************/

#ifndef SORTIX_MXFS_H
#define SORTIX_MXFS_H

#include "stream.h"
#include "filesystem.h"

namespace Sortix
{
	class DevMXFS;
	class DevMXFSFile;
	class MXFSNode;

	class DevMXFSFile : public DevBuffer
	{
		friend class DevMXFS;
		
	private:
		DevMXFSFile* _prevSibling;
		DevMXFSFile* _nextSibling;

	private:
		DevMXFS* _parent;

	private:
		intmax_t _beginning;
		intmax_t _size;

	public:
		virtual size_t Write(const void* buffer, size_t bufferSize) = 0;
		virtual size_t ReadSome(const void* buffer, size_t bufferSize) = 0;

	public:
		virtual size_t BlockSize() = 0;
		virtual intmax_t Size() = 0;
		virtual intmax_t Position() = 0;
		virtual bool Seek(intmax_t position) = 0;
		virtual bool Resize(intmax_t size) = 0;
	
	};

	class DevMXFS : public DevFileSystem
	{
		friend class DevMXFSFile;

	public:
		DevMXFS();
		virtual ~DevMXFS();

	public:
		virtual int Initialize(MountPoint* mountPoint, const char* commandLine);

	private:
		MountPoint* _mountPoint;
		DevBuffer* _storage;
		DevMXFSFile* _children;
		MXFSNode* _root;
	
	private:
		nat _state;
		uint64_t _parsingBlock;
		MXFSNode* _parsingLastNode;
		MXFSNode* _parsingParent;

	private:
		volatile int _storageStatus;
		intmax_t _blockId;
		uint8_t* _block;
		intmax_t _fsSize;

	private:
		uint8_t* GetBlock(intmax_t blockId);

	private:
		bool ParseHeader();
		bool ParseBlocks();
		void Failure();

	private:
		MXFSNode* Search(const char* path);

	protected:
		virtual void Think();

	public:
		virtual bool Sync();

	public:
		virtual Device* Open(const char* path, nat flags, nat permissions, nat* type);

	};
}

#endif


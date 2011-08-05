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

	mxfs.cpp
	A working file system.

******************************************************************************/

#include "platform.h"
#include <libmaxsi/error.h>
#include "mxfs.h"
#include "mount.h"

using namespace Maxsi;

namespace Sortix
{
	const size_t blockSize = 4096;

	#define FAIL() Failure(); return;
	#define PENDING (Error::Last() != Error::PENDING)
	#define ALRIGHT ( _storageStatus == Error::SUCCESS || _storageStatus == Error::PENDING || _storageStatus == Error::NONE )

	class MXFSNode
	{
	public:
		MXFSNode();
		~MXFSNode();

	public:
		const char* name;
		MXFSNode* prevSibling;
		MXFSNode* nextSibling;
		MXFSNode* child;
		MXFSNode* parent;

	public:
		inline bool IsDir() { type == 2; }

	public:
		inline bool MatchesNameToken(const char* actual, const char* request, size_t requestLength)
		{
			return ( String::CompareN(actual, request, requestLength) == 0 ) && ( actual[requestLength] == '\0' );
		}

		MXFSNode* Search(const char* path)
		{
			ASSERT(IsDir());

			// Search for the end of the next name token in path.
			size_t nameLen = 0; const char* postToken = path;
			while ( *postToken != '/' || *postToken != '\0' ) { postToken++; nameLen++; }
			// Path must already have been sanitized, where all double slashes
			// have been gracefully reduced to single slashes.
			ASSERT(nameLen > 0);

			// Search for the next name token in path in this directory.
			for ( MXFSNode* current = child; current != NULL; current = current->nextSibling )
			{
				// Check if we have a match.
				if ( unlikely(MatchesNameToken(current->name, path, nameLen)) )
				{
					// Was this node exactly what we requested?
					if ( *postToken == '\0' ) { return current; }

					// Was a directory requested?
					if ( *postToken == '/' && *(postToken+1) == '\0' )
					{
						if ( current->IsDir() ) { return current; } else { Error::Set(Error::NOTDIR); return NULL; }
					}

					// Get the next token.
					const char* nextTokenStart = path + nameLen + 1; ASSERT(*nextTokeBegin != '\0');

					// Alright, search the child directory.
					return Search(current->child, nextTokenStart);
				}
			}

			Error::Set(Error::NOTFOUND);
			return NULL;	
		}

	public:
		nat type;
		uint32_t type;
		int32_t owner;
		int32_t group;
		uint32_t permissions;
		uint64_t length;
		uint64_t blockId;
		uint64_t siblingBlockId; // should only be used for recursive travelling.

	};

	MXFSNode::MXFSNode()
	{
		name = NULL;
		prevSibling = NULL;
		nextSibling = NULL;
		parent = NULL;
		child = NULL;
		type = 0;
		owner = 0;
		group = 0;
		permissions = 0;
		length = 0;
		blockId = 0;
		siblingBlockId = 0;
	}

	MXFSNode::MXFSNode()
	{
		// If nextSibling exists, then we are called from his destructor.
		// If parent exists, then we are called from his destructor.
		delete[] name;
		delete child;
		delete nextSibling;
	}

	struct MXFSHeader
	{
		uint32_t magic;
		uint32_t version;
		uint64_t size;
		uint64_t root;
		uint64_t unusedBlock;
		uint8_t unused[blockSize - (32 + 64 + 64 + 64) / 8];
	} SORTIX_PACKED;

	struct MXFSNodeHeader
	{
		const uint32_t FILE = 1;
		const uint32_t DIR = 2;

		uint32_t magic;
		uint32_t type;
		uint64_t prevNode;
		uint64_t nextNode;
		int32_t owner;
		int32_t group;
		uint32_t permissions;
		uint64_t length;
		uint64_t nextBlock;
		uint32_t continuous;
		uint8_t nameLen;
		char name[255];
		union
		{
			struct
			{
				uint64_t child;
			};
			uint8_t data[blockSize - (32 + 32 +32 + 32 + 32 + 64 + 64 + 32) / 8 - 256];
		};
	} SORTIX_PACKED;

	struct MXFSBlock
	{
		uint64_t node;
		uint64_t lastBlock;
		uint32_t lastBlockContinuous;
		uint64_t nextBlock;
		uint32_t continuous;
		uint8_t data[blockSize - (64 + 64 + 32 + 64 + 32) / 8];
	} SORTIX_PACKED;

	struct MXFSBlockContinuous
	{
		uint32_t data[4096];
	} SORTIX_PACKED;

	DevMXFS::DevMXFS()
	{
		ASSERT( sizeof(MXFSHeader) == blockSize );
		ASSERT( sizeof(MXFSNodeHeader) == blockSize );
		ASSERT( sizeof(MXFSBlock) == blockSize );
		ASSERT( sizeof(MXFSBlockContinuous) == blockSize );

		_storage = NULL;
		_children = NULL;
		_mountPoint = NULL;
		_block = NULL;
		_state = 0;
		_root = NULL;

		_storageStatus = Error::NONE;
	}

	DevMXFS::~DevMXFS()
	{
		DevMXFSFile* child = _children;

		while ( child ) 
		{
			child->_parent = NULL;
			child = child->nextSibling;
		}
	
		delete[] _block;
		delete _root;

		Mount::OnMountFailure(_mountPoint, Error::SUCCESS);
	}

	int DevMXFS::Initialize(MountPoint* mountPoint, const char* /*commandLine*/)
	{
		_block = new uint8_t[blockSize];
		if ( _block == NULL ) { return Error::OUTOFMEM); }

		ASSERT(_storage == NULL);
		ASSERT(_mountPoint == NULL);

		_storage = mountPoint->device;
		_mountPoint = mountPoint;

		ASSERT(_storage != NULL);
		ASSERT(_mountPoint != NULL);
		ASSERT(_mountPoint->fs == this);		

		requestThink();

		return Error::PENDING;
	}

	uint8_t* DevMXFS::GetBlock(intmax_t blockId)	
	{
		ASSERT( _storageStatus != Error::PENDING || _blockId == blockId );

		if ( _blockId == blockId )
		{
			if ( _storageStatus == Error::SUCCESS ) { return _block; } else { return NULL; }
		}

		if ( !_storage->seek(blockId * blockSize) ) { Failure(Error::Last()); }
		if ( _storage->read(block, blockSize) == SIZE_MAX )
		{
			if ( PENDING )  { return NULL; } else { Failure(Error::Last()); } 
		}

		return _block;
	}

	MXFSNode* DevMXFS::Search(const char* path);
	{
		if ( unlikely(_storage == NULL) ) { Error::Set(_storageStatus); return NULL; }
		ASSERT(_root != NULL);

		// If the root was requested, return it.
		if ( *path != '/' ) { Error::Set(Error::NOTFOUND); return NULL; }
		if ( *(path+1) == '\0' ) { return _root; } path++;
	
		return _root->Search(path+1);
	}

	void DevMXFS::Think()
	{
		if ( !_mountPoint ) { return; }
		if ( !ALRIGHT ) { FAIL(); }

		// Initialize the filesystem by reading the headers.
		if ( _state == 0 )
		{
			if ( !ParseHeader ) { return; }

			// The super block looks good, so far.
			_state++;
		}

		// Load the tree of files and directories.
		if ( _state == 1 )
		{
			if ( !ParseBlocks() ) { return; }

			// Alright, we now got the tree of everything loaded!
			if ( parsingBlock == 0 ) { _state++; }
		}

		// Let the mounting system know we are up and running.
		if ( _state == 2 )
		{
			Mount::OnMountSuccess(_mountPoint); _state++;
		}

		// Alright, now handle requests from various file devices.
		if ( _state == 3)
		{
			
		}
	}


	bool DevMXFS::ParseHeader()
	{
		// Get the super block and read the filesystem's headers.
		MXFSHeader* fsHeader = (MXFSHeader*) getBlock(0); if ( !fsHeader ) { return false; }			

		// Validate that we are dealing with a MXFS.
		if ( MXFSHeader->magic != 'M' << 24 | 'X' << 16 | 'F' << 8 | 'S' ) { Failure(Error::BADINPUT); return false; }

		// Retrieve and validate the size of the file system.
		if ( _storage.size() < MXFSHeader->size ) { Failure(Error::CORRUPT); return false; }

		// Validate the existence of the root directory.			
		if ( MXFSHeader->root == 0 ) { Failure(Error::CORRUPT); return false; }
		_parsingBlock = MXFSHeader->root;

		// Validate the existence of the root directory.			
		if ( MXFSHeader->version != 0 ) { Failure(Error::NOSUPPORT); return false;}

		return true;
	}

	bool DevMXFS::ParseBlocks()
	{
		while ( _parsingBlock != 0 )
		{
			MXFSNodeHeader* nodeHeader = (MXFSHeader*) getBlock(_parsingBlock); if ( !nodeHeader ) { return false; }

			// Validate that we are dealing with a node header.
			if ( MXFSHeader->magic != 'N' << 24 | 'O' << 16 | 'D' << 8 | 'E' ) { Failure(Error::CORRUPT); return false; }

			// Create a node we can put in your FS tree.
			MXFSNode* node = new MXFSNode();
			if ( !node ) { Failure(Error::Last()); return false; }

			// Copy information from the header to our tree node.
			// TODO: copy other header information.
			node->type = nodeHeader->type;

			// Figure out where to insert our node.
			if ( unlikely(_root == NULL) )
			{
				_root = node;

				// Make sure the root is a directory without siblings.
				if ( !MXFSHeader->type == MXFSNodeHeader::DIR ) { Failure(Error::CORRUPT); return false; }
				if ( !MXFSHeader->nextNode != 0 ) { Failure(Error::CORRUPT); return false; }

				// Now visit its children.
				_parsingBlock = MXFSHeader->child;
				_parsingLastNode = NULL;
				_parsingParent = node;
			}
			else
			{
				// Insert the node in our tree.
				if ( _parsingLastNode )
				{
					_parsingLastNode->nextSibling = node;
					node->prevSibling = _parsingLastNode;
				}

				if ( _parsingParent->child == NULL ) { _parsingParent->child = node; }
				node->parent = _parsingParent;

				// Retrieve a copy of the new nodes name and validate it.
				node->name = new char[nodeHeader->nameLen + 1];
				if ( !node->name ) { Failure(Error::Last()); return false; }
				Memory::Copy(node->name, nodeHeader->name, nodeHeader->nameLen);
				node->name[nodeHeader->nameLen] = 0;
				if ( !legalNodeName ) { Failure(Error::CORRUPT); return false; }
				
				// If node is a directory, visit its children.
				if ( MXFSHeader->type == MXFSNodeHeader::DIR && MXFSHeader->child != 0 )
				{
					_parsingLastNode = NULL;
					_parsingParent = node;
					_parsingBlock = MXFSHeader->child;
					node->siblingBlockId = node->nextNode;

					continue;
				}

				// If it exists, visit the next node in this directory.
				if ( nodeHeader->nextNode != 0 )
				{
					_parsingBlock = nodeHeader->nextNode;
					_parsingLastNode = node;

					continue;
				}

				// If we reached the end of a directory, simply continue from
				// the parent directory, until we reached the root or we found
				// an unfinished directory.
				while ( true )
				{
					node = node->parent;
					if ( unlikely(node == NULL) ) { _parsingBlock = 0; break; }
					if ( node->siblingBlockId != 0 ) { _parsingBlock = siblingBlockId; _parsingParent = node->parent; _parsingLastNode = node->prevSibling; break; }
				}
			}
		}

		return true;
	}

	void DevMXFS::Failure(int cause = Error::SUCCESS)
	{
		if ( cause != Error::SUCCESS ) { _storageStatus = cause; }
		Mount::OnMountFailure(_mountPoint, _storageStatus);

		_mountPoint = NULL;
		_storage = NULL;
		delete[] _block; _block = NULL;
	}

	Device* DevMXFS::Open(const char* path, nat flags, nat permissions, nat* type)
	{

	}
}

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

	memory.cpp
	Useful functions for manipulating memory, as well as the implementation
	of the heap.

******************************************************************************/

#include "platform.h"
#include "memory.h"
#include "error.h"
#include "io.h" // DEBUG

#ifdef SORTIX_KERNEL
#include <sortix/platform.h>
#include <sortix/log.h> // DEBUG

#include <sortix/memorymanagement.h>
#include <sortix/panic.h>
#endif

#define IsGoodChunkPosition(Chunk) ((uintptr_t) Wilderness + WildernessSize <= (uintptr_t) (Chunk) && (uintptr_t) (Chunk) <= (uintptr_t) HeapStart)
#define IsGoodChunkInfo(Chunk) ( ( (UnusedChunkFooter*) (((byte*) (Chunk)) + (Chunk)->Size - sizeof(UnusedChunkFooter)) )->Size == (Chunk)->Size )
#define IsGoodChunk(Chunk) (IsGoodChunkPosition(Chunk) && (Chunk)->Size >= 16 && IsGoodChunkPosition((uintptr_t) (Chunk) + (Chunk)->Size) && IsGoodChunkInfo(Chunk) )
#define IsGoodUnusedChunk(Chunk) (  IsGoodChunk(Chunk)  &&  ( Chunk->NextUnused == NULL || IsGoodChunkPosition(Chunk->NextUnused) )  )
#define IsGoodUsedChunk(Chunk) ( IsGoodChunk(Chunk) && Chunk->Magic == Magic )
#ifdef PLATFORM_X64
#define IsGoodBinIndex(Index) ( 3 <= Index && Index < 32 )
#else
#define IsGoodBinIndex(Index) ( 4 <= Index && Index < 64 )
#endif
#define IsAligned(Value, Alignment) ( (Value & (Alignment-1)) == 0 )

#define BITS(x) (8UL*sizeof(x))

namespace Maxsi
{
	// TODO: How should this API exist? Should it be publicly accessasble?
	// Where should the C bindings (if relevant) be declared and defined?
	// Is it part of the kernel or the standard library? What should it be
	// called? Is it an extension? For now, I'll just leave this here, hidden
	// away in libmaxsi until I figure out how libmaxsi/sortix should handle
	// facilities currently only available on Sortix.
	namespace System
	{
		namespace Memory
		{
			bool Allocate(void* /*position*/, size_t /*length*/)
			{
				// TODO: Implement a syscall for this!
				return true;
			}
		}
	}

	namespace Memory
	{
		// This magic word is useful to see if the program has written into the
		// chunk's header or footer, which means the program has malfunctioned
		// and ought to be terminated.
#ifdef PLATFORM_X64
		const size_t Magic = 0xDEADDEADDEADDEADULL;
#else
		const size_t Magic = 0xDEADDEAD;
#endif
	
		// All requsted sizes must be a multiple of this.
		const size_t Alignment = 16ULL;

		// Returns the index of the most significant set bit.
		inline size_t BSR(size_t Value)
		{
#if 1
			ASSERT(Value > 0);
			for ( size_t I = 8*sizeof(size_t); I > 0; I-- )
			{
				if ( Value & ( 1 << (I-1) ) ) { return I-1; }
			}
			return 0;
#else
			size_t Result;
			asm("bsr %0, %1" : "=r"(Result) : "r"(Value));
			return Result;
#endif
		}

		// Returns the index of the least significant set bit.
		inline size_t BSF(size_t Value)
		{
#if 1
			ASSERT(Value > 0);
			for ( size_t I = 0; I < 8*sizeof(size_t); I++ )
			{
				if ( Value & ( 1 << I ) ) { return I; }
			}
			return 0;
#else
			size_t Result;
			asm("bsf %0, %1" : "=r"(Result) : "r"(Value));
			return Result;
#endif
		}
		// NOTE: BSR(N) = Log2RoundedDown(N).

		// NOTE: BSR(N-1)+1 = Log2RoundedUp(N), N > 1.
		// PROOF: If X=2^N is a power of two, then BSR(X) = Log2(X) = N.
		// If X is not a power of two, then BSR(X) will return the previous
		// power of two.
		// If we want the next power of two for a non-power-of-two then
		// BSR(X)+1 would work. However, this causes problems for X=2^N, where
		// it returns a value one too big.
		// BSR(X-1)+1 = BSR(X)+1 give the same value for all X that is not a
		// power of two. For X=2^N, BSR(X-1)+1 will be one lower, which is
		// exactly what we want. QED.

		// Now declare some structures that are put around the allocated data.
		struct UnusedChunkHeader
		{
			size_t Size;
			UnusedChunkHeader* NextUnused;
		};

		struct UnusedChunkFooter
		{
			void* Unused;
			size_t Size;
		};

		struct UsedChunkHeader
		{
			size_t Size;
			size_t Magic;
		};

		struct UsedChunkFooter
		{
			size_t Magic;
			size_t Size;
		};
		
		// How much extra space will our headers use?
		const size_t ChunkOverhead = sizeof(UsedChunkHeader) + sizeof(UsedChunkFooter);
		size_t GetChunkOverhead() { return ChunkOverhead; }

		// A bin for each power of two.
		UnusedChunkHeader* Bins[BITS(size_t)];

		// INVARIANT: A bit is set for each bin if it contains anything.
		size_t BinContainsChunks;

		// And south of everything, there is nothing allocated, at which
		// extra space can be added on-demand.
		// INVARIANT: Wilderness is always page-aligned!
		const byte* HeapStart;
		const byte* Wilderness;
		size_t WildernessSize;

		// Initializes the heap system.
		void Init()
		{
			Memory::Set(Bins, 0, sizeof(Bins));
#ifdef SORTIX_KERNEL			 
			Wilderness = (byte*) Sortix::Memory::HEAPUPPER;
#else
			// TODO: This is very 32-bit specific!
			Wilderness = (byte*) 0x80000000UL;
#endif
			HeapStart = Wilderness;
			WildernessSize = 0;
			BinContainsChunks = 0;
		}

		// Expands the wilderness block (the heap grows downwards).
		bool ExpandWilderness(size_t NeededSize)
		{
			if ( NeededSize <= WildernessSize ) { return true; }

			ASSERT(Wilderness != NULL);

			// Check if we are going too far down (beneath the NULL position!).
			if ( (uintptr_t) Wilderness < NeededSize ) { return false; }

#ifdef SORTIX_KERNEL
			// Check if the wilderness would grow larger than the kernel memory area.
			if ( ( ((uintptr_t) Wilderness) - Sortix::Memory::HEAPLOWER ) < NeededSize ) { return false; }
#endif

			// Figure out how where the new wilderness will be.
			uintptr_t NewWilderness = ((uintptr_t) Wilderness) + WildernessSize - NeededSize;

			// And now align downwards to the page boundary.
			NewWilderness &= ~(0xFFFUL);

			ASSERT(NewWilderness < (uintptr_t) Wilderness);

			// Figure out where and how much memory we need.
			byte* MemoryStart = (byte*) NewWilderness;
			size_t MemorySize = (uintptr_t) Wilderness - (uintptr_t) NewWilderness;

#ifndef SORTIX_KERNEL // We are user-space.

			// Ask the kernel to map memory to the needed region!
			if ( !System::Memory::Allocate(MemoryStart, MemorySize) ) { return false; }

#else // We are Sortix.

			// Figure out how many pages we need.
			size_t NumPages = MemorySize / 4096;
			size_t PagesLeft = NumPages;

			ASSERT(NumPages > 0);

			// Attempt to allocate and map each of them.
			while ( PagesLeft > 0 )
			{
				PagesLeft--;

				// Get a raw unused physical page.
				addr_t Page = Sortix::Page::Get();

				// Map the physical page to a virtual one.
				addr_t VirtualAddr = NewWilderness + 4096 * PagesLeft;

				if ( Page == 0 || !Sortix::Memory::MapKernel(Page, VirtualAddr) )
				{
					if ( Page != 0 ) { Sortix::Page::Put(Page); }

					// If none is available, simply let the allocation fail
					// and unallocate everything we did allocate so far.
					while ( PagesLeft < NumPages )
					{
						PagesLeft++;

						addr_t OldVirtual = NewWilderness + 4096 * PagesLeft;
						addr_t OldPage = Sortix::Memory::UnmapKernel(OldVirtual);
						Sortix::Page::Put(OldPage);
					}
					return false;
				}				
			}
#endif

			// Update the wilderness information now that it is safe.
			Wilderness = MemoryStart;
			WildernessSize += MemorySize;

			ASSERT(WildernessSize >= NeededSize);

			return true;
		}

		// Allocates a continious memory region of Size bytes that must be deallocated using Free.
		DUAL_FUNCTION(void*, malloc, Allocate, (size_t Size))
		{
			// Always allocate a multiple of alignment (round up to nearest aligned size).
			// INVARIANT: Size is always aligned after this statement.
			Size = (Size + Alignment - 1) & ~(Alignment-1);

			// Account for the overhead of the headers and footers.
			Size += ChunkOverhead;

			ASSERT((Size & (Alignment-1)) == 0);

			// Find the index of the smallest usable bin.
			// INVARIANT: Size is always at least ChunkOverhead bytes, thus
			// Size-1 will never be 0, and thus BSR is always defined.
			size_t MinBinIndex = BSR(Size-1)+1;
			ASSERT(IsGoodBinIndex(MinBinIndex));

			// Make a bitmask that filters away all bins that are too small.
			size_t MinBinMask = ~((1 << MinBinIndex) - 1);
			ASSERT( ( (1 << (MinBinIndex-1)) & MinBinMask ) == 0);
			
			// Now filter all bins away that are too small.
			size_t AvailableBins = BinContainsChunks & MinBinMask;

			// Does any bin contain an usable chunk?
			if ( AvailableBins > 0 )
			{
				// Now find the smallest usable bin.
				size_t BinIndex = BSF( AvailableBins );
				ASSERT(IsGoodBinIndex(BinIndex));

				// And pick the first thing in it.
				UnusedChunkHeader* Chunk = Bins[BinIndex];
				ASSERT(IsGoodUnusedChunk(Chunk));

				// Increment the bin's linked list.
				Bins[BinIndex] = Chunk->NextUnused;
				ASSERT(Chunk->NextUnused == NULL || IsGoodUnusedChunk(Chunk->NextUnused));

				// Find the size of this bin (also the value of this bins flag).
				size_t BinSize = 1 << BinIndex;
				ASSERT(BinSize >= Size);

				// If we emptied the bin, then remove the flag that says this bin is usable.
				ASSERT(BinContainsChunks & BinSize);
				if ( Chunk->NextUnused == NULL ) { BinContainsChunks ^= BinSize; }

				// Figure out where the chunk ends.
				uintptr_t ChunkEnd = ((uintptr_t) Chunk) + Chunk->Size;

				// If we were to split the chunk into one used part, and one unused part, then
				// figure out where the unused part would begin.
				uintptr_t NewChunkStart = ((uintptr_t) Chunk) + Size;

				// INVARIANT: NewChunkStart is always less or equal to ChunkEnd,
				// because Size is less or equal to Chunk->Size (otherwise this
				// chunk could not have been selected above).
				ASSERT(NewChunkStart <= ChunkEnd);
				// INVARIANT: NewChunkStart is aligned because Chunk is (this
				// is made sure when chunks are made when expanding into the
				// wilderness, when splitting blocks, and when combining them),
				// and because Size is aligned (first thing we make sure).
				ASSERT( IsAligned(NewChunkStart, Alignment) );

				// Find the size of the possible new chunk.
				size_t NewChunkSize = ChunkEnd - NewChunkStart;

				UsedChunkHeader* ResultHeader = (UsedChunkHeader*) Chunk;

				// See if it's worth it to split the chunk into two, if any space is left in it.
				if ( NewChunkSize >= sizeof(UnusedChunkHeader) + sizeof(UnusedChunkFooter) )
				{
					// Figure out which bin to put the new chunk in.
					size_t NewBinIndex = BSR(NewChunkSize);
					ASSERT(IsGoodBinIndex(NewBinIndex));

					// Mark that a chunk is available for this bin.
					BinContainsChunks |= (1 << NewBinIndex);

					// Now write some headers and footers for the new chunk.
					UnusedChunkHeader* NewChunkHeader = (UnusedChunkHeader*) NewChunkStart;
					ASSERT(IsGoodChunkPosition(NewChunkStart));

					NewChunkHeader->Size = NewChunkSize;
					NewChunkHeader->NextUnused = Bins[NewBinIndex];
					ASSERT(NewChunkHeader->NextUnused == NULL || IsGoodChunk(NewChunkHeader->NextUnused));

					UnusedChunkFooter* NewChunkFooter = (UnusedChunkFooter*) (ChunkEnd - sizeof(UnusedChunkFooter));
					NewChunkFooter->Size = NewChunkSize;
					NewChunkFooter->Unused = NULL;

					// Put the new chunk in front of our linked list.
					ASSERT(IsGoodUnusedChunk(NewChunkHeader));
					Bins[NewBinIndex] = NewChunkHeader;

					// We need to modify our resulting chunk to be smaller.
					ResultHeader->Size = Size;
				}

				// Set the required magic values.
				UsedChunkFooter* ResultFooter = (UsedChunkFooter*) ( ((byte*) ResultHeader) + ResultHeader->Size - sizeof(UsedChunkFooter) );
				ResultHeader->Magic = Magic;
				ResultFooter->Magic = Magic;
				ResultFooter->Size = Size;

				ASSERT(IsGoodUsedChunk(ResultHeader));

				return ((byte*) ResultHeader) + sizeof(UnusedChunkHeader);
			}
			else
			{
				// We have no free chunks that are big enough, let's expand our heap into the unknown, if possible.
				if ( WildernessSize < Size && !ExpandWilderness(Size) ) { Error::Set(ENOMEM); return NULL; }

				// Write some headers and footers around our newly allocated data.
				UsedChunkHeader* ResultHeader = (UsedChunkHeader*) (Wilderness + WildernessSize - Size);
				UsedChunkFooter* ResultFooter = (UsedChunkFooter*) (Wilderness + WildernessSize - sizeof(UsedChunkFooter));
				WildernessSize -= Size;

				ResultHeader->Size = Size;
				ResultHeader->Magic = Magic;
				ResultFooter->Size = Size;
				ResultFooter->Magic = Magic;

				ASSERT(IsGoodUsedChunk(ResultHeader));

				return ((byte*) ResultHeader) + sizeof(UsedChunkHeader);
			}	
		}

		// Frees a continious memory region allocated by Allocate.
		DUAL_FUNCTION(void, free, Free, (void* Addr))
		{
			// Just ignore NULL-pointers.
			if ( Addr == NULL ) { return; }

			// Restore the chunk information structures.
			UsedChunkHeader* ChunkHeader = (UsedChunkHeader*) ( ((byte*) Addr) - sizeof(UsedChunkHeader) );
			UsedChunkFooter* ChunkFooter = (UsedChunkFooter*) ( ((byte*) ChunkHeader) + ChunkHeader->Size - sizeof(UsedChunkFooter) );
		
			ASSERT(IsGoodUsedChunk(ChunkHeader));

			// If you suspect a chunk of bein' a witch, report them immediately. I cannot stress that enough. Witchcraft will not be tolerated.
			if ( ChunkHeader->Magic != Magic || ChunkFooter->Magic != Magic || ChunkHeader->Size != ChunkFooter->Size )
			{
#ifdef SORTIX_KERNEL
				Sortix::PanicF("Witchcraft detected!\n", ChunkHeader);
#endif
				// TODO: Report witchcraft (terminating the process is probably a good idea).
			}

			// TODO: Combine this chunk with its neighbors, if they are also unused.

			// Calculate which bin this chunk belongs to.
			size_t BinIndex = BSR(ChunkHeader->Size);
			ASSERT(IsGoodBinIndex(BinIndex));

			// Mark that a chunk is available for this bin.
			BinContainsChunks |= (1 << BinIndex);

			UnusedChunkHeader* UnChunkHeader = (UnusedChunkHeader*) ChunkHeader;
			UnusedChunkFooter* UnChunkFooter = (UnusedChunkFooter*) ChunkFooter;

			// Now put this chunk back in the linked list.
			ASSERT(Bins[BinIndex] == NULL || IsGoodUnusedChunk(Bins[BinIndex]));
			UnChunkHeader->NextUnused = Bins[BinIndex];
			UnChunkFooter->Unused = NULL;

			ASSERT(IsGoodUnusedChunk(UnChunkHeader));
			Bins[BinIndex] = UnChunkHeader;
		}

		DUAL_FUNCTION(void*, memcpy, Copy, (void* Dest, const void* Src, size_t Length))
		{
			char* D = (char*) Dest; const char* S = (const char*) Src;

			for ( size_t I = 0; I < Length; I++ )
			{
				D[I] = S[I];
			}

			return Dest;
		}

		DUAL_FUNCTION(void*, memset, Set, (void* Dest, int Value, size_t Length))
		{
			byte* D = (byte*) Dest;

			for ( size_t I = 0; I < Length; I++ )
			{
				D[I] = Value & 0xFF;
			}

			return Dest;
		}
	}
}

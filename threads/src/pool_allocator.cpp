/*
This file is part of the DistFieldHexMesh application/library.

	The DistFieldHexMesh application/library is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	The DistFieldHexMesh application/library is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	This link provides the exact terms of the GPL license <https://www.gnu.org/licenses/>.

	The author's interpretation of GPL 3 is that if you receive money for the use or distribution of the DistFieldHexMesh application/library or a derivative product, GPL 3 no longer applies.

	Under those circumstances, the author expects and may legally pursue a reasoble share of the income. To avoid the complexity of agreements and negotiation, the author makes
	no specific demands in this regard. Compensation of roughly 1% of net or $5 per user license seems appropriate, but is not legally binding.

	In lay terms, if you make a profit by using the DistFieldHexMesh application/library (violating the spirit of Open Source Software), I expect a reasonable share for my efforts.

	Robert R Tipton - Author

	Dark Sky Innovative Solutions http://darkskyinnovation.com/
*/

#include <defines.h>
#include <assert.h>
#include <algorithm>
#include <pool_allocator.h>

namespace
{

static ::MultiCore::local_heap s_mainThreadHeap(4 * 1024);
static thread_local ::MultiCore::local_heap* s_pHeap = &s_mainThreadHeap;

}

void ::MultiCore::local_heap::pushThreadHeapPtr(::MultiCore::local_heap* pHeap)
{
	if (s_pHeap)
		s_pHeap->_priorHeap = s_pHeap;
	s_pHeap = pHeap;
}

void ::MultiCore::local_heap::popThreadHeapPtr()
{
	if (s_pHeap)
		s_pHeap = s_pHeap->_priorHeap;
}

::MultiCore::local_heap* ::MultiCore::local_heap::getThreadHeapPtr()
{
	return s_pHeap;
}

::MultiCore::local_heap::local_heap(size_t blockSizeChunks, size_t chunkSizeBytes)
	: _blockSizeChunks(blockSizeChunks)
	, _chunkSizeBytes(chunkSizeBytes + sizeof(BlockHeader))
{
	_data.reserve(10);
}

void* ::MultiCore::local_heap::allocMem(size_t bytes)
{
	size_t bytesNeeded = bytes + sizeof(BlockHeader);
	size_t numChunks = bytesNeeded / _chunkSizeBytes;
	if (bytesNeeded % _chunkSizeBytes != 0)
		numChunks++;

	BlockHeader* pHeader;
#if 1
	pHeader = getAvailBlock(numChunks);
	if (pHeader != nullptr) {
		return (char*)pHeader + sizeof(BlockHeader);
	}
#endif
	assert(numChunks < _blockSizeChunks);

	if (_topBlockIdx >= _data.size() || (_topChunkIdx + numChunks >= _blockSizeChunks)) {
		// Not enough room in the block, so make an empty one.

		if (_topChunkIdx + numChunks >= _blockSizeChunks) {
			// Store the empty space for the next allocation
			BlockHeader headerForRemainder;
			headerForRemainder._blockIdx = _topBlockIdx;
			headerForRemainder._chunkIdx = (uint32_t) _topChunkIdx;
			headerForRemainder._numChunks = (uint32_t) (_blockSizeChunks - _topChunkIdx);
			addBlockToAvailList(headerForRemainder);
		}

		_topChunkIdx = 0;

		size_t blockSize = _blockSizeChunks * _chunkSizeBytes;
		auto pBlk = _STD make_shared<_STD vector<char>>(blockSize);
		_data.push_back(pBlk);
	}

	size_t startIdx = _topChunkIdx * _chunkSizeBytes;
	
	auto& blkVec = *_data[_topBlockIdx];
	pHeader = (BlockHeader*) &blkVec[startIdx];
	pHeader->_numChunks = (uint32_t)numChunks;
	pHeader->_blockIdx = _topBlockIdx;
	pHeader->_chunkIdx = _topChunkIdx;
	pHeader->_size = (uint32_t)bytes;

	_topChunkIdx += (uint32_t) numChunks;
	if (_topChunkIdx >= _blockSizeChunks) {
		_topBlockIdx++;
		_topChunkIdx = 0;
	}

	assert(pHeader->_size == bytes);
	return (char*)pHeader + sizeof(BlockHeader);
}

void ::MultiCore::local_heap::freeMem(void* ptr)
{
	if (ptr) {
		assert(isHeaderValid(ptr, false));
		char* pc = (char*)ptr - sizeof(BlockHeader);
		assert(isHeaderValid(pc, true));
		BlockHeader* pHeader = (BlockHeader*)pc;
		addBlockToAvailList(*pHeader);
	}
}

::MultiCore::local_heap::BlockHeader* ::MultiCore::local_heap::getAvailBlock(size_t numChunksNeeded)
{
	AvailBlockHeader *pCurChunk = _pFirstAvailBlock;
	AvailBlockHeader* pPriorChunk = nullptr;
	while (pCurChunk) {
		if (numChunksNeeded <= pCurChunk->_header._numChunks) {
			// Remove this chunk from the sorted linked list of available chunks
			if (pCurChunk == _pFirstAvailBlock) {
				_pFirstAvailBlock = pCurChunk->_pNext;
			} else {
				assert(pPriorChunk);
				pPriorChunk->_pNext = pCurChunk->_pNext;
			}

			size_t startIdxBytes = pCurChunk->_header._chunkIdx * _chunkSizeBytes;
			auto& blkVec = *_data[pCurChunk->_header._blockIdx];
			BlockHeader* pHeader = (BlockHeader*)&blkVec[startIdxBytes];
			(*pHeader) = pCurChunk->_header;
			return pHeader;
		}
		pPriorChunk = pCurChunk;
		pCurChunk = pCurChunk->_pNext;
	}

	return nullptr;
}

void ::MultiCore::local_heap::addBlockToAvailList(const BlockHeader& header)
{
	AvailBlockHeader* pCurChunk = _pFirstAvailBlock;
	AvailBlockHeader* pPriorChunk = nullptr;

	size_t startIdxBytes = header._chunkIdx * _chunkSizeBytes;
	auto& blkVec = *_data[header._blockIdx];
	AvailBlockHeader* pNewChunk = (AvailBlockHeader*)&blkVec[startIdxBytes];
	assert(pNewChunk->_header == header);
	pNewChunk->_header = header;
	pNewChunk->_header._size = -1; // mark as a free block
	pNewChunk->_pNext = nullptr;

	bool found = false;
	while (pCurChunk) {
		if (header._numChunks <= pCurChunk->_header._numChunks) {
			found = true;
			// insert here
			if (pCurChunk == _pFirstAvailBlock) {
				pNewChunk->_pNext = _pFirstAvailBlock;
				if (_pFirstAvailBlock)
					_pFirstAvailBlock->_pNext = pNewChunk;
				else
					_pFirstAvailBlock = pNewChunk;
			} else {
				assert(pPriorChunk);
				pNewChunk->_pNext = pCurChunk->_pNext;
				pPriorChunk->_pNext = pNewChunk;
			}
			break;
		}

		pPriorChunk = pCurChunk;
		pCurChunk = pCurChunk->_pNext;
	}

	if (!found) {
		if (!_pFirstAvailBlock) {
			pNewChunk->_pNext = _pFirstAvailBlock;
			_pFirstAvailBlock = pNewChunk;
		} else {
			assert(pPriorChunk);
			pPriorChunk->_pNext = pNewChunk;
		}
	}
}

bool ::MultiCore::local_heap::isHeaderValid(const void* p, bool pointsToHeader) const
{
	const char* pc = (const char*) p;
	if (!pointsToHeader)
		pc -= sizeof(BlockHeader);

	const BlockHeader* pHeader = (const BlockHeader*)pc;
	if (pHeader->_numChunks == 0 || pHeader->_numChunks >= _blockSizeChunks)
		return false;

	if (pHeader->_size != -1 /* -1 marks an empty block*/ &&
		(pHeader->_size == 0 || pHeader->_size >= _blockSizeChunks * _chunkSizeBytes))
		return false;

	if (pHeader->_blockIdx >= _data.size())
		return false;

	if (pHeader->_chunkIdx >= _blockSizeChunks)
		return false;

	return true;
}

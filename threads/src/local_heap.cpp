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
#include <local_heap.h>

namespace
{

static ::MultiCore::local_heap s_mainThreadHeap(4 * 1024);
static thread_local ::MultiCore::local_heap* s_pHeap = &s_mainThreadHeap;

}

void ::MultiCore::local_heap::setThreadHeapPtr(::MultiCore::local_heap* pHeap)
{
	s_pHeap = pHeap;
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

void* ::MultiCore::local_heap::allocMem(size_t numBytes)
{
	size_t bytesNeeded = numBytes + sizeof(BlockHeader);
	size_t numChunks = bytesNeeded / _chunkSizeBytes;
	if (bytesNeeded % _chunkSizeBytes != 0)
		numChunks++;

	BlockHeader* pHeader = getAvailBlock(numChunks);
	if (pHeader != nullptr) {
		return (char*)pHeader + sizeof(BlockHeader);
	}

	bool createdNewBlock = false;
	size_t blockChunks = _blockSizeChunks;
	if (!_data.empty()) {
		size_t customChunks = _data.back()->size() / _chunkSizeBytes;
		if (_data.back()->size() % _chunkSizeBytes != 0)
			customChunks += 1;
		if (customChunks > _blockSizeChunks)
			blockChunks = customChunks;
	}

	if (_topBlockIdx >= _data.size() || (_topChunkIdx + numChunks >= blockChunks)) {
		// Not enough room in the block, so make an empty one.

		if (_topChunkIdx + numChunks >= _blockSizeChunks) {
			// Store the empty space for the next allocation
			BlockHeader headerForRemainder;
			headerForRemainder._blockIdx = _topBlockIdx;
			headerForRemainder._chunkIdx = (uint32_t) _topChunkIdx;
			headerForRemainder._numChunks = (uint32_t) (_blockSizeChunks - _topChunkIdx);
			if (headerForRemainder._numChunks > 0)
				addBlockToAvailList(headerForRemainder);
			createdNewBlock = true;
		}

		size_t blockSize = _blockSizeChunks * _chunkSizeBytes;
		if (bytesNeeded > blockSize) {
			size_t chk = bytesNeeded / _chunkSizeBytes;
			if (bytesNeeded % _chunkSizeBytes != 0)
				chk += 1;
			bytesNeeded = chk * _chunkSizeBytes;
			blockSize = bytesNeeded;
		}
		auto pBlk = _STD make_shared<_STD vector<char>>(blockSize);

		_topBlockIdx = (uint32_t) _data.size();
		_topChunkIdx = 0;
		_data.push_back(pBlk);
		assert(_topBlockIdx < _data.size());
	}

	size_t startIdx = _topChunkIdx * _chunkSizeBytes;
	
	auto& blkVec = *_data[_topBlockIdx];
	pHeader = (BlockHeader*) &blkVec[startIdx];
	pHeader->_numChunks = (uint32_t)numChunks;
	pHeader->_blockIdx = _topBlockIdx;
	pHeader->_chunkIdx = _topChunkIdx;

	_topChunkIdx += (uint32_t) numChunks;

	return (char*)pHeader + sizeof(BlockHeader);
}

void ::MultiCore::local_heap::freeMem(void* ptr)
{
	if (ptr) {
		char* pc = (char*)ptr - sizeof(BlockHeader);
		BlockHeader* pHeader = (BlockHeader*)pc;
		addBlockToAvailList(*pHeader);
	}
}

::MultiCore::local_heap::BlockHeader* ::MultiCore::local_heap::getAvailBlock(size_t numChunksNeeded)
{
	BlockHeader* pResult = nullptr;
	AvailBlockHeader *pCurBlock = _pFirstAvailBlock;
	AvailBlockHeader* pPriorBlock = _pFirstAvailBlock;
	while (pCurBlock) {
		if (numChunksNeeded <= pCurBlock->_header._numChunks) {
			removeAvailBlock(pPriorBlock, pCurBlock);

			size_t startIdxBytes = pCurBlock->_header._chunkIdx * _chunkSizeBytes;
			auto& blkVec = *_data[pCurBlock->_header._blockIdx];
			BlockHeader* pHeader = (BlockHeader*)&blkVec[startIdxBytes];
			(*pHeader) = pCurBlock->_header;
			pResult = pHeader;

			break;
		}
		pPriorBlock = pCurBlock;
		pCurBlock = pCurBlock->_pNext;
	}

	return pResult;
}

void ::MultiCore::local_heap::addBlockToAvailList(const BlockHeader& header)
{
	AvailBlockHeader* pCurBlock = _pFirstAvailBlock;
	AvailBlockHeader* pPriorBlock = _pFirstAvailBlock;

	size_t startIdxBytes = header._chunkIdx * _chunkSizeBytes;
	auto& blkVec = *_data[header._blockIdx];
	AvailBlockHeader* pNewBlock = (AvailBlockHeader*)&blkVec[startIdxBytes];
	pNewBlock->_header = header;
	pNewBlock->_pNext = nullptr;

	while (pCurBlock) {
		if (header._numChunks <= pCurBlock->_header._numChunks) {
			break;
		}

		pPriorBlock = pCurBlock;
		pCurBlock = pCurBlock->_pNext;
	}

	insertAvailBlock(pPriorBlock, nullptr, pNewBlock);
}

void MultiCore::local_heap::insertAvailBlock(AvailBlockHeader* pPriorBlock, AvailBlockHeader* pCurBlock, AvailBlockHeader* pNewBlock)
{
	if (!_pFirstAvailBlock) {
		// create list of one entry
		_pFirstAvailBlock = pNewBlock;
		return;
	} else if (pPriorBlock == _pFirstAvailBlock) {
		if (pNewBlock->_header._numChunks <= _pFirstAvailBlock->_header._numChunks) {
			// Put new at the head
			pNewBlock->_pNext = _pFirstAvailBlock;
			_pFirstAvailBlock = pNewBlock;
		} else {
			pNewBlock->_pNext = _pFirstAvailBlock->_pNext;
			_pFirstAvailBlock->_pNext = pNewBlock;
		}
		assert(_pFirstAvailBlock != _pFirstAvailBlock->_pNext);
		assert(_pFirstAvailBlock->_header._numChunks <= pNewBlock->_header._numChunks);
	} else if (pPriorBlock && pCurBlock) {
		pNewBlock->_pNext = pCurBlock;
		pPriorBlock->_pNext = pNewBlock;

		assert(pNewBlock != pNewBlock->_pNext);
		assert(pPriorBlock != pPriorBlock->_pNext);
		assert(pPriorBlock->_header._numChunks <= pNewBlock->_header._numChunks);
		assert(pNewBlock->_header._numChunks <= pCurBlock->_header._numChunks);
	} else if (!pCurBlock) {
		// add at the end
		assert(pPriorBlock);
		pNewBlock->_pNext = pPriorBlock->_pNext;
		pPriorBlock->_pNext = pNewBlock;

		assert(pNewBlock != pNewBlock->_pNext);
		assert(pPriorBlock != pPriorBlock->_pNext);
		assert(pPriorBlock->_header._numChunks <= pNewBlock->_header._numChunks);
	}
}

void ::MultiCore::local_heap::removeAvailBlock(AvailBlockHeader* pPriorBlock, AvailBlockHeader* pDeadBlock)
{

	if (pDeadBlock == _pFirstAvailBlock) {
		_pFirstAvailBlock = pDeadBlock->_pNext;
	} else if (pPriorBlock && pDeadBlock) {
		pPriorBlock->_pNext = pDeadBlock->_pNext;
	} else if (!pDeadBlock) {
		// add at the end
		pPriorBlock->_pNext = nullptr;
	}
}

bool ::MultiCore::local_heap::isHeaderValid(const void* p, bool pointsToHeader) const
{
	const char* pc = (const char*) p;
	if (!pointsToHeader)
		pc -= sizeof(BlockHeader);

	const BlockHeader* pHeader = (const BlockHeader*)pc;
	size_t startIdx = pHeader->_chunkIdx * _chunkSizeBytes;
	size_t chunkSize = pHeader->_numChunks * _chunkSizeBytes;
	size_t sizeNeeded = startIdx + chunkSize;

	const auto& pBlk = _data[pHeader->_blockIdx];
	if (pHeader->_numChunks == 0 || sizeNeeded > pBlk->size())
		return false;

	if (pHeader->_blockIdx >= _data.size())
		return false;

	size_t blockSizeChunks = (size_t)(pBlk->size() / _chunkSizeBytes);
	if (pHeader->_chunkIdx >= blockSizeChunks)
		return false;

	return true;
}

bool MultiCore::local_heap::verifyAvailList() const
{
	auto pCurBlock = _pFirstAvailBlock;
	if (!isPointerInBounds(pCurBlock))
		return false;

	while (pCurBlock) {
		if (!isBlockValid(pCurBlock))
			return false;

		if (!isPointerInBounds(pCurBlock->_pNext))
			return false;
		if (pCurBlock->_pNext) {
			if (pCurBlock == pCurBlock->_pNext)
				return false;
			if (pCurBlock->_header._numChunks > pCurBlock->_pNext->_header._numChunks)
				return false;
		}
		pCurBlock = pCurBlock->_pNext;
	}

	return true;
}

bool ::MultiCore::local_heap::isBlockValid(const AvailBlockHeader* pBlock) const
{
	if (!pBlock)
		return true;

	if (pBlock->_header._numChunks == 0)
		return false;

	if (!isPointerInBounds(pBlock))
		return false;
	if ((pBlock->_pNext != nullptr) && !isPointerInBounds(pBlock->_pNext))
		return false;

	for (const auto& blkPtr : _data) {
		const auto& blks = *blkPtr;
		size_t idx = ((size_t)pBlock) - ((size_t)blks.data());
		if (idx < blks.size()) {
			size_t chunkIdx = idx / _chunkSizeBytes;
			if (chunkIdx != pBlock->_header._chunkIdx)
				return false;
			return true;
		}
	}
	return false;
}

bool ::MultiCore::local_heap::isPointerInBounds(const void* ptr) const
{
	if (!ptr)
		return true;

	for (const auto& blkPtr : _data) {
		const auto& blks = *blkPtr;
		size_t idx = ((size_t)ptr) - ((size_t)blks.data());
		if (idx < blks.size()) {
			return true;
		}
	}
	return false;
}

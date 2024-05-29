#pragma once
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

#include <memory>
#include <vector>
#include <list>

namespace MultiCore
{

/*
	This class's purpose is to create a heap manager which is local to an object and/or thread.
	Since DFHM's threading is 1-1 on blocks, if the block owns the localHeap all memory allocations within that block
	are within the thread local heap - no mutex conflicts on new/delete.

	It is only possible to REDUCE access to the application heap, not eliminate it totally.

	Justification - during early testing, single threaded was faster than multi threaded and the process was only getting 15% of available CPU.
	In other testing, the app got access to 100% of available CPU.

	The heap for the main thread is set on start up.
	When processing a block, the block's heap is pushed on the heap stack using scoped_set_thread_heap
		If the block is in the main thread, it's heap is used for ALL operations. When local variables are destroyed, they are removed from the block's heap
		If the block is in a different thread, it's heap is used for ALL operations IN THAT THREAD, making the active block the thread local heap. Local variables are destroyed on exit.

	Net result is, all operations in a thread have their own heap as well as their own stack. Persistant data is stored in the block.
	A bit fragile, but usable.

	I tried using the std memory pool system, but it didn't come anywhere close to the required speed.
*/

class local_heap {
public:
	static void setThreadHeapPtr(local_heap* pHeap);
	static local_heap* getThreadHeapPtr();

	class scoped_set_thread_heap {
	public:
		scoped_set_thread_heap(local_heap* pHeap);
		~scoped_set_thread_heap();

	private:
		local_heap* _priorHeap = nullptr;
	};

	local_heap(size_t blockSizeChunks, size_t chunkSizeBytes = 32);
	
	template<class T>
	T* alloc(size_t num);

	template<class T>
	void free(T* ptr);

private:	
	struct BlockHeader {
		uint32_t _numChunks;
		uint32_t _blockIdx;
		uint32_t _chunkIdx;
		uint32_t _numObj = 0;

		inline bool operator == (const BlockHeader& rhs) const
		{
			return _blockIdx == rhs._blockIdx && _chunkIdx == rhs._chunkIdx && _numChunks == rhs._numChunks;
		}
	};

	struct AvailBlockHeader {
		BlockHeader _header;
		AvailBlockHeader* _pNext = nullptr;
	};

	void* allocMem(size_t bytes);

	void freeMem(void* ptr);

	BlockHeader* getAvailBlock(size_t numChunksNeeded);
	void addBlockToAvailList(const BlockHeader& header);
	void insertAvailBlock(AvailBlockHeader* pPriorBlock, AvailBlockHeader* pCurBlock, AvailBlockHeader* pNewBlock);
	void removeAvailBlock(AvailBlockHeader* pPriorBlock, AvailBlockHeader* pDeadBlock);

	bool isHeaderValid(const void* p, bool pointsToHeader) const;
	bool verifyAvailList() const;
	bool isBlockValid(const AvailBlockHeader* pBlock) const;
	bool isPointerInBounds(const void* ptr) const;

	const size_t _blockSizeChunks;
	const size_t _chunkSizeBytes;

	uint32_t _topBlockIdx = 0;
	uint32_t _topChunkIdx = 0;

	using BlockPtr = std::shared_ptr<_STD vector<char>>;
	_STD vector<BlockPtr> _data;

	AvailBlockHeader* _pFirstAvailBlock = nullptr; // Sorted indices into _availChunks
};

template<class T>
T* local_heap::alloc(size_t num)
{
	assert(verifyAvailList());
	char* pc = (char*)allocMem(num * sizeof(T));
	auto pT = (T*)pc;
	assert(isHeaderValid(pT, false));

	assert(verifyAvailList());
	BlockHeader* pHeader = (BlockHeader*)(pc - sizeof(BlockHeader));
	pHeader->_numObj = (uint32_t)num;
	for (size_t i = 0; i < num; i++) {
		T* px = new(&pT[i]) T(); // default in place constructor
		assert(px == &pT[i]);
	}

	assert(isHeaderValid(pT, false));

	assert(verifyAvailList());
	return pT;
}

template<class T>
void local_heap::free(T* ptr)
{
	if (ptr) {
		assert(isHeaderValid(ptr, false));
		assert(verifyAvailList());

		char* pc = (char*)ptr;
		const BlockHeader* pHeader = (BlockHeader*)(pc - sizeof(BlockHeader));
		size_t num = pHeader->_numObj;
		for (size_t i = 0; i < num; i++)
			ptr[i].~T();

		assert(isHeaderValid(ptr, false));
		assert(verifyAvailList());
		freeMem(ptr);
	}
}

class local_heap_user
{
protected:
	template<class T>
	T* alloc(size_t num) const
	{
		return getHeap()->alloc<T>(num);
	}

	template<class T>
	void free(T* ptr) const
	{
		getHeap()->free(ptr);
	}

private:
	local_heap* getHeap() const;

	mutable local_heap* _pOurHeap = nullptr;

};

inline local_heap::scoped_set_thread_heap::scoped_set_thread_heap(local_heap* pHeap)
{

	_priorHeap = local_heap::getThreadHeapPtr();
	local_heap::setThreadHeapPtr(pHeap);
}

inline local_heap::scoped_set_thread_heap::~scoped_set_thread_heap()
{
	setThreadHeapPtr(_priorHeap);
}

inline local_heap* local_heap_user::getHeap() const
{
	// We assign the heap on the first usage
	if (!_pOurHeap)
		_pOurHeap = local_heap::getThreadHeapPtr();

	return _pOurHeap;
}

}


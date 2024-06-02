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

#define EXPENSIVE_ASSERT_ON 0
#define GUARD_BAND_SIZE 0
#define NUM_AVAIL_SIZE 256

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
	
	void clear();

	template<class T>
	T* alloc(size_t num);

	template<class T>
	void free(T*& ptr);

#if GUARD_BAND_SIZE > 0
	struct GuardBand {
		size_t _b0[GUARD_BAND_SIZE];
		GuardBand* _pEndBand = nullptr;
		size_t _b1[GUARD_BAND_SIZE];

		inline GuardBand() {
			for (size_t i = 0; i < GUARD_BAND_SIZE; i++) {
				_b0[i] = 0xdeadbeefdeadbeef;
				_b1[i] = 0xdeadbeefdeadbeef;
			}
		}

		inline GuardBand(const GuardBand& src) {
			for (size_t i = 0; i < GUARD_BAND_SIZE; i++) {
				assert(src._b0[i] == 0xdeadbeefdeadbeef);
				assert(src._b1[i] == 0xdeadbeefdeadbeef);
				_b0[i] = 0xdeadbeefdeadbeef;
				_b1[i] = 0xdeadbeefdeadbeef;
			}
		}

		inline GuardBand& operator = (const GuardBand& rhs) {
			for (size_t i = 0; i < GUARD_BAND_SIZE; i++) {
				assert(_b0[i] == 0xdeadbeefdeadbeef);
				assert(rhs._b0[i] == 0xdeadbeefdeadbeef);
				_b0[i] = 0xdeadbeefdeadbeef;
				assert(_b1[i] == 0xdeadbeefdeadbeef);
				assert(rhs._b1[i] == 0xdeadbeefdeadbeef);
				_b1[i] = 0xdeadbeefdeadbeef;
			}
			return *this;
		}

		inline bool isValid() const {
			for (size_t i = 0; i < GUARD_BAND_SIZE; i++) {
				if (_b0[i] != 0xdeadbeefdeadbeef)
					return false;
				if (_b1[i] != 0xdeadbeefdeadbeef)
					return false;
			}
			if (_pEndBand)
				return _pEndBand->isValid();
			return true;
		}
	};
#endif

	bool verify() const;
private:	
	struct BlockHeader {
		BlockHeader() = default;
		BlockHeader(const BlockHeader& src) = default;

		uint32_t _numChunks;
		uint32_t _blockIdx;
		uint32_t _chunkIdx;
		uint32_t _numObj = 0;
#if GUARD_BAND_SIZE > 0
		GuardBand _leadingBand;
#endif


		inline bool operator == (const BlockHeader& rhs) const
		{
			return _blockIdx == rhs._blockIdx && _chunkIdx == rhs._chunkIdx && _numChunks == rhs._numChunks;
		}
	};

	struct AvailBlockHeader {
		AvailBlockHeader() = default;
		AvailBlockHeader(const AvailBlockHeader& src) = default;
		AvailBlockHeader(const BlockHeader& src)
			: _header(src)
			, _pNext(nullptr)
		{}
		BlockHeader _header;
		AvailBlockHeader* _pNext = nullptr;
	};

	void* allocMem(size_t bytes);

	template<class P>
	void freeMem(P*& ptr);

	BlockHeader* getAvailBlock(size_t numChunksNeeded);
	void addBlockToAvailList(const BlockHeader& header);
	void insertAvailBlock(AvailBlockHeader*& pFirstAvailBlock, AvailBlockHeader* pPriorBlock, AvailBlockHeader* pCurBlock, AvailBlockHeader* pAvailBlock);
	void removeAvailBlock(AvailBlockHeader*& pFirstAvailBlock, AvailBlockHeader* pPriorBlock, AvailBlockHeader* pRecycledBlock);

	bool isHeaderValid(const void* p, bool pointsToHeader) const;
	bool verifyAvailList() const;
	bool isAvailBlockValid(const AvailBlockHeader* pBlock) const;
	bool isPointerInBounds(const void* ptr) const;
	bool isBlockAvail(const BlockHeader* pHeader) const;

	AvailBlockHeader*& getFirstAvailBlockPtr(size_t numChunksNeeded) const;

	const size_t _blockSizeChunks;
	const size_t _chunkSizeBytes;

	using BlockPtr = std::shared_ptr<_STD vector<char>>;
	_STD vector<BlockPtr> _data;

	uint32_t _topBlockIdx = 0;
	uint32_t _topChunkIdx = 0;

	mutable AvailBlockHeader* _pFirstAvailBlockTable[NUM_AVAIL_SIZE]; // Sorted indices into _availChunks
};

template<class T>
T* local_heap::alloc(size_t num)
{
	char* pc = (char*)allocMem(num * sizeof(T));
	auto pT = (T*)pc;

	BlockHeader* pHeader = (BlockHeader*)(pc - sizeof(BlockHeader));
	pHeader->_numObj = (uint32_t)num;
	for (size_t i = 0; i < num; i++) {
		T* px = new(&pT[i]) T(); // default in place constructor
	}

#if GUARD_BAND_SIZE > 0
	assert(pHeader->_leadingBand.isValid());
#endif
	return pT;
}

template<class T>
void local_heap::free(T*& ptr)
{
	if (ptr) {

		char* pc = (char*)ptr;
		BlockHeader* pHeader = (BlockHeader*)(pc - sizeof(BlockHeader));
#if GUARD_BAND_SIZE > 0
		assert(pHeader->_leadingBand.isValid());
#endif
		size_t num = pHeader->_numObj;
		for (size_t i = 0; i < num; i++)
			ptr[i].~T();

		pHeader->_numObj = 0;
		freeMem(ptr);
		ptr = nullptr;
	}
}

template<class P>
void ::MultiCore::local_heap::freeMem(P*& ptr)
{
	if (ptr) {
		char* pc = (char*)ptr - sizeof(BlockHeader);
		BlockHeader* pHeader = (BlockHeader*)pc;
#if GUARD_BAND_SIZE > 0
		assert(pHeader->_leadingBand.isValid());
#endif
		addBlockToAvailList(*pHeader);
		ptr = nullptr;
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
	void free(T*& ptr) const
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


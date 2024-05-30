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

#include <map>
#include <pool_vector.h>
#include <pool_set.h>

#define FORW_CONST 0
#define REV_CONST 1
#define FORW 2
#define REV 3
#define CONST (IterType < FORW)

namespace MultiCore
{

template<class KEY, class T>
class map {
public:
	using pairRec = std::pair<KEY, T>;
	using DataMap = MultiCore::map<KEY, T>;
	using DataVec = ::MultiCore::vector<pairRec>;

	struct KeyRec
	{
		size_t _idx = -1;
		DataVec* _pVec;

		KeyRec(const KeyRec& src) = default;
		inline KeyRec(DataVec* pVec = nullptr, size_t idx = -1)
			: _pVec(pVec)
			, _idx(idx)
		{
		}

		inline bool operator < (const KeyRec& rhs) const
		{
			assert(_pVec == rhs._pVec);

			if (_idx < _pVec->size() && rhs._idx < _pVec->size())
				return (*_pVec)[_idx].first < (*rhs._pVec)[rhs._idx]valRhs.first;
			else if (_idx < _pVec->size())
				return true; // rhs is out of bounds



		}

	};

	using KeySet = ::MultiCore::set<KeyRec>;
	using KeyIndexVec = ::MultiCore::vector<size_t>;

private:

	template <int IterType>
	class _iterator
	{
	public:
		friend class MultiCore::map;

		using iterator_category = std::random_access_iterator_tag;
		using difference_type = std::ptrdiff_t;

		using value_type = std::remove_cv_t<T>;
		using KeyIter = std::conditional_t<IterType == FORW || IterType == FORW_CONST, typename KeySet::const_iterator, typename KeySet::const_reverse_iterator>;
		using pointer = std::conditional_t<CONST, pairRec const*, pairRec*>;
		using reference = std::conditional_t<CONST, pairRec const&, pairRec&>;

		_iterator() = default;
		_iterator(DataMap* pSource, const KeyIter& keyIter, pointer pEntry);
		_iterator(const _iterator& src) = default;

		bool operator == (const _iterator& rhs) const;
		bool operator != (const _iterator& rhs) const;
		bool operator < (const _iterator& rhs) const;
		bool operator > (const _iterator& rhs) const;

		_iterator& operator ++ ();
		_iterator& operator --();
		_iterator& operator ++ (int);
		_iterator& operator --(int);

		_iterator operator + (size_t val) const;

		_iterator operator - (size_t val) const;
		size_t operator - (const _iterator& rhs) const;

		reference operator *() const;
		pointer operator->() const;
		pointer get() const;

	private:
		void refreshDataPointer();
		pointer _pEntry = nullptr;
		DataMap* _pSource;
		KeyIter _keyIter;
	};

public:
	using iterator = _iterator<FORW>;
	using const_iterator = _iterator<FORW_CONST>;
	using reverse_iterator = _iterator<REV>;
	using const_reverse_iterator = _iterator<REV_CONST>;

	map() = default;
	map(const map& src) = default;
	~map() = default;

	bool empty() const;
	size_t size() const;
	void clear();
	const pairRec* data() const;
	pairRec* data() ;

	std::pair<iterator, bool> insert(const pairRec& pair);

	void erase(const const_iterator& at);

	map& operator = (const map& rhs);

	_NODISCARD _CONSTEXPR20 iterator begin() noexcept;
	_NODISCARD _CONSTEXPR20 iterator end() noexcept;
	_NODISCARD _CONSTEXPR20 const_iterator begin() const noexcept;
	_NODISCARD _CONSTEXPR20 const_iterator end() const noexcept;

	_NODISCARD _CONSTEXPR20 reverse_iterator rbegin() noexcept;
	_NODISCARD _CONSTEXPR20 reverse_iterator rend() noexcept;
	_NODISCARD _CONSTEXPR20 const_reverse_iterator rbegin() const noexcept;
	_NODISCARD _CONSTEXPR20 const_reverse_iterator rend() const noexcept;

	_NODISCARD _CONSTEXPR20 iterator find(const KEY& val) noexcept;
	_NODISCARD _CONSTEXPR20 const_iterator find(const KEY& val) const noexcept;
	bool contains(const KEY& val) const;

protected:
	_NODISCARD _CONSTEXPR20 iterator find(const KEY& val, const_iterator& next) noexcept;
	_NODISCARD _CONSTEXPR20 const_iterator find(const KEY& val, const_iterator& next) const noexcept;

private:
	pairRec* allocEntry(const pairRec& pair);
	void releaseEntry(const pairRec* pData);

	::MultiCore::vector<pairRec> _data;
	::MultiCore::set<KeyRec> _keySet;
	::MultiCore::vector<size_t> _availEntries;

#if DUPLICATE_STD_TESTS	
	std::map<KEY, DATA> _map;
#endif
};

}
#include <pool_map.hpp>

#undef FORW_CONST
#undef REV_CONST
#undef FORW
#undef REV
#undef CONST

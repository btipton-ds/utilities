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

#include <local_heap.h>
#include <pool_map.h>

#define TEMPL_DECL template<class KEY, class T>
#define ITER_TEMPL_DECL template <int IterType>
#define MAP_DECL MultiCore::map<KEY, T>
#define ITER_DECL MultiCore::map<KEY, T>::_iterator<IterType>

TEMPL_DECL
std::pair<typename MAP_DECL::iterator, bool> MAP_DECL::insert(const pairRec& pair)
{
	auto keyIter = _keySet.find(KeyRec(&_data, pair.first)); // Confused about value vs index. Must be able to compare and index for a pair that isn't in the array yet.
	if (keyIter == _keySet.end()) {
		auto* pPair = allocEntry(pair);
		size_t idx = (size_t)(pPair - _data.data());

		KeyRec rec(&_data, idx);

		keyIter = _keySet.insert(rec);

		return std::pair(iterator(this, keyIter, pPair), true);
	}

	const KeyRec& keyRec = *keyIter;
	auto pPair = &_data[keyRec._idx];
	return std::pair(iterator(this, keyIter, pPair), false);
}

TEMPL_DECL
void MAP_DECL::erase(const const_iterator& at)
{
	pairRec* p = at.get();
	size_t idx = (size_t)(p - _data.data());
	*p = pairRec();
	_keySet.erase(at._keyIter);
	_availEntries.push_back(idx);
}

TEMPL_DECL
inline bool MAP_DECL::empty() const
{
	return _keySet.empty();
}

TEMPL_DECL
inline size_t MAP_DECL::size() const
{
	return _keySet.size();
}

TEMPL_DECL
void MAP_DECL::clear()
{
	_keySet.clear();
	_data.clear();
}

TEMPL_DECL
_NODISCARD _CONSTEXPR20 inline typename MAP_DECL::iterator MAP_DECL::find(const KEY& val) noexcept
{
	return _keySet.find(val);
}

TEMPL_DECL
_NODISCARD _CONSTEXPR20 inline typename MAP_DECL::const_iterator MAP_DECL::find(const KEY& val) const noexcept
{
	return _keySet.find(val);
}

TEMPL_DECL
typename MAP_DECL::pairRec* MAP_DECL::allocEntry(const pairRec& pair)
{
	if (_availEntries.empty()) {
		_data.push_back(pair);
		return &_data.back();
	} else {
		size_t idx = _availEntries.back();
		_availEntries.pop_back();
		_data[idx] = pair;
		return &_data[idx];
	}
}

TEMPL_DECL
void MAP_DECL::releaseEntry(const pairRec* pPair)
{
	if (!pPair)
		return;

	*pPair = pairRec();
	size_t idx = (size_t)(pPair - _data.data());
	_availEntries.push_back(idx);
}

TEMPL_DECL
_NODISCARD _CONSTEXPR20 inline typename MAP_DECL::iterator MAP_DECL::begin() noexcept
{
	return iterator(this, _keySet.begin(), &_data[0]);
}

TEMPL_DECL
_NODISCARD _CONSTEXPR20 inline typename MAP_DECL::iterator MAP_DECL::end() noexcept
{
	return iterator(this, _keySet.end(), nullptr);
}

TEMPL_DECL
_NODISCARD _CONSTEXPR20 inline typename MAP_DECL::const_iterator MAP_DECL::begin() const noexcept
{
	return iterator(this, _keySet.begin(), &_data[0]);
}

TEMPL_DECL
_NODISCARD _CONSTEXPR20 inline typename MAP_DECL::const_iterator MAP_DECL::end() const noexcept
{
	return iterator(this, _keySet.end(), nullptr);
}

TEMPL_DECL
_NODISCARD _CONSTEXPR20 inline typename MAP_DECL::reverse_iterator MAP_DECL::rbegin() noexcept
{
	return reverse_iterator(this, _keySet.rbegin(), &_data.back());
}

TEMPL_DECL
_NODISCARD _CONSTEXPR20 inline typename MAP_DECL::reverse_iterator MAP_DECL::rend() noexcept
{
	return reverse_iterator(this, _keySet.rend(), nullptr);
}

TEMPL_DECL
_NODISCARD _CONSTEXPR20 inline typename MAP_DECL::const_reverse_iterator MAP_DECL::rbegin() const noexcept
{
	return reverse_iterator(this, _keySet.rbegin(), &_data.back());
}

TEMPL_DECL
_NODISCARD _CONSTEXPR20 inline typename MAP_DECL::const_reverse_iterator MAP_DECL::rend() const noexcept
{
	return reverse_iterator(this, _keySet.rend(), nullptr);
}

TEMPL_DECL
inline const typename MAP_DECL::pairRec* MAP_DECL::data() const
{
	return _data.data();
}

TEMPL_DECL
inline typename MAP_DECL::pairRec* MAP_DECL::data()
{
	return _data.data();
}

/*************************************************************************************************************/
/*************************************************************************************************************/

TEMPL_DECL
ITER_TEMPL_DECL
ITER_DECL::_iterator(DataMap* pSource, const KeyIter& keyIter, pointer pEntry)
	: _pSource(pSource)
	, _keyIter(keyIter)
	, _pEntry(pEntry)
{
}

TEMPL_DECL
ITER_TEMPL_DECL
inline bool ITER_DECL::operator == (const _iterator& rhs) const
{
	return _keyIter == rhs._keyIter;
}

TEMPL_DECL
ITER_TEMPL_DECL
inline bool ITER_DECL::operator != (const _iterator& rhs) const
{
	return _keyIter != rhs._keyIter;
}

TEMPL_DECL
ITER_TEMPL_DECL
inline bool ITER_DECL::operator < (const _iterator& rhs) const
{
	return _keyIter < rhs._keyIter;
}

TEMPL_DECL
ITER_TEMPL_DECL
inline bool ITER_DECL::operator > (const _iterator& rhs) const
{
	return _keyIter > rhs._keyIter;
}

TEMPL_DECL
ITER_TEMPL_DECL
inline void ITER_DECL::refreshDataPointer()
{
	const KeyRec* pKey = _keyIter.get();
	if (pKey && pKey->_idx < _pSource->size()) {
		auto& dataArray = _pSource->_data;
		_pEntry = &dataArray[pKey->_idx];
	} else
		_pEntry = nullptr;
}

TEMPL_DECL
ITER_TEMPL_DECL
inline typename ITER_DECL::_iterator& ITER_DECL::operator ++ ()
{
	_keyIter.operator++();
	refreshDataPointer();
	return *this;
}

TEMPL_DECL
ITER_TEMPL_DECL
inline typename ITER_DECL::_iterator& ITER_DECL::operator --()
{
	_keyIter.operator--();
	refreshDataPointer();
	return *this;
}

TEMPL_DECL
ITER_TEMPL_DECL
inline typename ITER_DECL::_iterator& ITER_DECL::operator ++ (int)
{
	_keyIter.operator++(1);
	refreshDataPointer();
	return *this;
}

TEMPL_DECL
ITER_TEMPL_DECL
inline typename ITER_DECL::_iterator& ITER_DECL::operator --(int)
{
	_keyIter.operator--(1);
	refreshDataPointer();
	return *this;
}

TEMPL_DECL
ITER_TEMPL_DECL
inline typename ITER_DECL::_iterator ITER_DECL::operator + (size_t val) const
{
	_iterator result(this);
	result._keyIter = _keyIter.operator+(val);
	result.refreshDataPointer();
	return result;
}

TEMPL_DECL
ITER_TEMPL_DECL
inline typename ITER_DECL::_iterator ITER_DECL::operator - (size_t val) const
{
	_iterator result(this);
	result._keyIter = _keyIter.operator-(val);
	result.refreshDataPointer();
	return result;
}

TEMPL_DECL
ITER_TEMPL_DECL
inline size_t ITER_DECL::operator - (const _iterator& rhs) const
{
	return _keyIter.operator-(rhs._keyIter);
}

TEMPL_DECL
ITER_TEMPL_DECL
inline typename ITER_DECL::reference ITER_DECL::operator *() const
{
	return *get();
}

TEMPL_DECL
ITER_TEMPL_DECL
inline typename ITER_DECL::pointer ITER_DECL::operator->() const
{
	return get();
}

TEMPL_DECL
ITER_TEMPL_DECL
inline typename ITER_DECL::pointer ITER_DECL::get() const
{
	return _pEntry ? _pEntry : _pSource ? _pSource->data() : nullptr;
}

#undef TEMPL_DECL 
#undef MAP_DECL 

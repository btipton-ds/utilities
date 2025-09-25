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
#define ITER_DECL_RET MultiCore::map<KEY, T>::template _iterator<IterType>

TEMPL_DECL
std::pair<typename MAP_DECL::iterator, bool> MAP_DECL::insert(const DataPair& pair)
{
	auto keyIter = _keySet.find(KeyRec(pair.first)); // Confused about value vs index. Must be able to compare and index for a pair that isn't in the array yet.
	if (keyIter == _keySet.end()) {
		auto* pPair = allocEntry(pair);
		size_t idx = (size_t)(pPair - _data.data());

		KeyRec rec(pair.first, &_data, idx);

		keyIter = _keySet.insert(rec);

		iterator newIter(this, keyIter);
		return std::pair(newIter, true);
	}

	iterator newIter(this, keyIter);
	return std::pair(newIter, false);
}

TEMPL_DECL
void MAP_DECL::erase(const iterator& at)
{
	DataPair* p = const_cast<DataPair*> (at.get());
	size_t idx = (size_t)(p - _data.data());
	*p = DataPair();

	_keySet.erase(at._keyIter);
	if (_keySet.empty()) {
		_data.clear();
		_availEntries.clear();
	} else if (idx == _data.size() - 1)
		_data.pop_back();
	else
		_availEntries.push_back(idx);
}

TEMPL_DECL
void MAP_DECL::erase(const const_iterator& at)
{
	DataPair* p = const_cast<DataPair*> (at.get());
	size_t idx = (size_t)(p - _data.data());
	*p = DataPair();
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
size_t MAP_DECL::count(const KEY& key) const
{
	return _keySet.count(key);
}

TEMPL_DECL
bool MAP_DECL::contains(const KEY& key)
{
	return _keySet.contains(key);
}

TEMPL_DECL
inline typename MAP_DECL::iterator MAP_DECL::find(const KEY& val) noexcept
{
	auto keyIter = _keySet.find(KeyRec(val));
	return iterator(this, keyIter);
}

TEMPL_DECL
inline typename MAP_DECL::const_iterator MAP_DECL::find(const KEY& val) const noexcept
{
	auto keyIter = _keySet.find(KeyRec(val));
	return const_iterator(this, keyIter);
}

TEMPL_DECL
typename MAP_DECL::DataPair* MAP_DECL::allocEntry(const DataPair& pair)
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
void MAP_DECL::releaseEntry(const DataPair* pPair)
{
	if (!pPair)
		return;

	*pPair = DataPair();
	size_t idx = (size_t)(pPair - _data.data());
	_availEntries.push_back(idx);
}

TEMPL_DECL
const T& MAP_DECL::operator[](const KEY& key) const
{
	return find(key)->second;
}

TEMPL_DECL
T& MAP_DECL::operator[](const KEY& key)
{
	if (find(key) == end())
		insert(make_pair(key, T()));
	return find(key)->second;
}

TEMPL_DECL
inline typename MAP_DECL::iterator MAP_DECL::begin() noexcept
{
	return iterator(this, _keySet.begin());
}

TEMPL_DECL
inline typename MAP_DECL::iterator MAP_DECL::end() noexcept
{
	return iterator(this, _keySet.end());
}

TEMPL_DECL
inline typename MAP_DECL::const_iterator MAP_DECL::begin() const noexcept
{
	typename MultiCore::set<KeyRec>::const_iterator iter = _keySet.begin();
	return const_iterator(this, iter);
}

TEMPL_DECL
inline typename MAP_DECL::const_iterator MAP_DECL::end() const noexcept
{
	return const_iterator(this, _keySet.end());
}

TEMPL_DECL
inline typename MAP_DECL::reverse_iterator MAP_DECL::rbegin() noexcept
{
	return reverse_iterator(this, _keySet.rbegin());
}

TEMPL_DECL
inline typename MAP_DECL::reverse_iterator MAP_DECL::rend() noexcept
{
	return reverse_iterator(this, _keySet.rend());
}

TEMPL_DECL
inline typename MAP_DECL::const_reverse_iterator MAP_DECL::rbegin() const noexcept
{
	return reverse_iterator(this, _keySet.rbegin());
}

TEMPL_DECL
inline typename MAP_DECL::const_reverse_iterator MAP_DECL::rend() const noexcept
{
	return reverse_iterator(this, _keySet.rend());
}

TEMPL_DECL
inline const typename MAP_DECL::DataPair* MAP_DECL::data() const
{
	return _data.data();
}

TEMPL_DECL
inline typename MAP_DECL::DataPair* MAP_DECL::data()
{
	return _data.data();
}

/*************************************************************************************************************/
/*************************************************************************************************************/

TEMPL_DECL
ITER_TEMPL_DECL
ITER_DECL::_iterator(dataMap* pSource, const KeyIter& keyIter)
	: _pSource(pSource)
	, _keyIter(keyIter)
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
inline typename ITER_DECL_RET& ITER_DECL::operator ++ ()
{
	_keyIter.operator++();
	return *this;
}

TEMPL_DECL
ITER_TEMPL_DECL
inline typename ITER_DECL_RET& ITER_DECL::operator --()
{
	_keyIter.operator--();
	return *this;
}

TEMPL_DECL
ITER_TEMPL_DECL
inline typename ITER_DECL_RET ITER_DECL::operator ++ (int)
{
	_iterator tmp(*this);
	++*this;
	return tmp;
}

TEMPL_DECL
ITER_TEMPL_DECL
inline typename ITER_DECL_RET ITER_DECL::operator --(int)
{
	_iterator tmp(*this);
	--*this;
	return tmp;
}

TEMPL_DECL
ITER_TEMPL_DECL
inline typename ITER_DECL_RET ITER_DECL::operator + (size_t val) const
{
	_iterator result(_pSource, _keyIter.operator+(val));
	return result;
}

TEMPL_DECL
ITER_TEMPL_DECL
inline typename ITER_DECL_RET ITER_DECL::operator - (size_t val) const
{
	_iterator result(_pSource, _keyIter.operator-(val));
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
inline const MAP_DECL::DataPair& ITER_DECL::operator *() const
{
	return *get();
}

TEMPL_DECL
ITER_TEMPL_DECL
inline typename MAP_DECL::DataPair& ITER_DECL::operator *()
{
	return *get();
}

TEMPL_DECL
ITER_TEMPL_DECL
inline const typename MAP_DECL::DataPair* ITER_DECL::operator->() const
{
	return get();
}

TEMPL_DECL
ITER_TEMPL_DECL
inline typename MAP_DECL::DataPair* ITER_DECL::operator->()
{
	return get();
}

TEMPL_DECL
ITER_TEMPL_DECL
inline const typename MAP_DECL::DataPair* ITER_DECL::get() const
{
	const KeyRec& key = *_keyIter;
	if (key._pVec && key._idx < key._pVec->size()) {
		auto& arr = *key._pVec;
		return &arr[key._idx];
	}
	return nullptr;
}

TEMPL_DECL
ITER_TEMPL_DECL
inline typename MAP_DECL::DataPair* ITER_DECL::get()
{
	const KeyRec& key = *_keyIter;
	if (key._pVec && key._idx < key._pVec->size()) {
		auto& arr = *key._pVec;
		return &arr[key._idx];
	}
	return nullptr;
}

TEMPL_DECL
inline MAP_DECL::KeyRec::KeyRec(const KEY& key, DataVec* pVec, size_t idx)
	: _key(key)
	, _pVec(pVec)
	, _idx(idx)
{
}

TEMPL_DECL
inline bool MAP_DECL::KeyRec::operator < (const KeyRec& rhs) const
{
	if (_pVec && rhs._pVec && _idx < _pVec->size() && rhs._idx < rhs._pVec->size())
		return (*_pVec)[_idx].first < (*rhs._pVec)[rhs._idx].first;
	else if (_pVec && _idx < _pVec->size())
		return (*_pVec)[_idx].first < rhs._key;
	else if (rhs._pVec && rhs._idx < rhs._pVec->size())
		return _key < (*rhs._pVec)[rhs._idx].first;

	return _key < rhs._key;
}

#undef TEMPL_DECL 
#undef MAP_DECL 

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

#include <set>
#include <pool_vector.h>

namespace MultiCore
{
template<class KEY, class T>
class map;

template<class T>
class set : private vector<T> {
private:
	template<class KEY, class TT>
	friend class map;

public:
	using const_iterator = vector<T>::const_iterator;
	using const_reverse_iterator = vector<T>::const_reverse_iterator;

	set() = default;
	set(const set& src) = default;
	set(const MultiCore::vector<T>& src);
	set(const std::set<T>& src);
	set(const std::initializer_list<T>& src);
	~set();

	operator std::set<T>() const;

	bool empty() const;
	size_t size() const;
	void clear();

	const_iterator insert(const T& val);
	void insert(const std::initializer_list<T>& vals);
	template<class ITER_TYPE>
	void insert(const ITER_TYPE& begin, const ITER_TYPE& end);

	void erase(const T& val);
	void erase(const const_iterator& at);
	void erase(const const_iterator& begin, const const_iterator& end);

	set& operator = (const set& rhs);

	const_iterator begin() const noexcept;
	const_iterator end() const noexcept;

	const_reverse_iterator rbegin() const noexcept;
	const_reverse_iterator rend() const noexcept;

	const_iterator find(const T& val) const noexcept;
	bool contains(const T& val) const;
	size_t count(const T& val) const;

protected:
	const_iterator find(const T& val, const_iterator& next) const noexcept;

private:

#if DUPLICATE_STD_TESTS	
	std::set<T> _set;
#endif
};

}

#include <pool_set.hpp>


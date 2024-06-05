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

#include <vector>
#include <local_heap.h>

#define FORW_CONST 0
#define REV_CONST 1
#define FORW 2
#define REV 3
#define IS_ITER_CONST (IterType < FORW)

namespace MultiCore
{

template<class T>
class vector : private local_heap_user {
protected:

template <int IterType>
class _iterator
	{
	public:
		friend class MultiCore::vector;

		using iterator_category = std::random_access_iterator_tag;
		using difference_type = std::ptrdiff_t;

#if 1
		using value_type = std::remove_cv_t<T>;
		using pointer = std::conditional_t<IS_ITER_CONST, T const*, T*>;
		using reference = std::conditional_t<IS_ITER_CONST, T const&, T&>;
#else
		using value_type = T;
		using pointer = T*;
		using reference = T&;
#endif
		_iterator() = default;
		_iterator(const MultiCore::vector<T>* pSource, pointer pEntry);
		_iterator(const _iterator& src) = default;

		bool operator == (const _iterator& rhs) const;
		bool operator != (const _iterator& rhs) const;
		bool operator < (const _iterator& rhs) const;
		bool operator > (const _iterator& rhs) const;

		_iterator& operator ++ ();		// prefix
		_iterator& operator --();		// prefix
		_iterator operator ++ (int);	// postfix
		_iterator operator --(int);		// postfix

		_iterator operator + (size_t val) const;

		_iterator operator - (size_t val) const;
		size_t operator - (const _iterator& rhs) const;

		reference operator *() const;
		pointer operator->() const;
		pointer get() const;

	private:
		pointer _pEntry; // Resizing/reserving invalidates the iterator. Don't keep them across these calls
		MultiCore::vector<T>* _pSource;
	};

public:
	using iterator = _iterator<FORW>;
	using const_iterator = _iterator<FORW_CONST>;
	using reverse_iterator = _iterator<REV>;
	using const_reverse_iterator = _iterator<REV_CONST>;

	vector();
	vector(const MultiCore::vector<T>& src);
	explicit vector(const std::vector<T>& src);
	vector(const std::initializer_list<T>& src);
	~vector();

	operator std::vector<T>() const;

	void clear();
	bool empty() const;
	size_t size() const;
	void resize(size_t val);
	void reserve(size_t val);

	iterator insert(const iterator& at, const T& val);
	const_iterator insert(const const_iterator& at, const T& val);

	template<class ITER_TYPE>
	void insert(const iterator& at, const ITER_TYPE& begin, const ITER_TYPE& end);
	void insert(const iterator& at, const std::initializer_list<T>& vals);

	iterator erase(const iterator& at);
	const_iterator erase(const const_iterator& at);
	iterator erase(const iterator& begin, const iterator& end);

	vector& operator = (const MultiCore::vector<T>& rhs);
//	vector& operator = (const std::vector<T>& rhs);

	_NODISCARD _CONSTEXPR20 const_iterator begin() const noexcept;
	_NODISCARD _CONSTEXPR20 iterator begin() noexcept;
	_NODISCARD _CONSTEXPR20 const_iterator end() const noexcept;
	_NODISCARD _CONSTEXPR20 iterator end() noexcept;

	_NODISCARD _CONSTEXPR20 const_reverse_iterator rbegin() const noexcept;
	_NODISCARD _CONSTEXPR20 reverse_iterator rbegin() noexcept;
	_NODISCARD _CONSTEXPR20 const_reverse_iterator rend() const noexcept;
	_NODISCARD _CONSTEXPR20 reverse_iterator rend() noexcept;

	const T* data() const;
	T* data();

	const T& front() const;
	T& front();
	const T& back() const;
	T& back();

	const T& operator[](size_t idx) const;
	T& operator[](size_t idx);

	size_t push_back(const T& val);
	void pop_back();

private:
	size_t _size = 0, _capacity = 0;
	T* _pData = nullptr;
};

}

#include <pool_vector.hpp>

#undef FORW_CONST
#undef REV_CONST
#undef FORW
#undef REV
#undef IS_ITER_CONST

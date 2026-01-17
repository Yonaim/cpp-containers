/**
 * SUMMARY: partial specialization for vector<bool>
 * https://en.cppreference.com/w/cpp/container/vector_bool.html
 *
 * : bool 타입 객체를 연속적으로 모아두는 대신 bool 타입 객체 하나를 1비트로 치환하여 저장 *
 */

#ifndef FT_BVECTOR_H
#define FT_BVECTOR_H

#include <climits>
#include <iterator>
#include "ft_memory.h"
#include "iterator.h"
#include "type_traits.h"
#include "vector.h"

// =============================== Bit_reference ===============================

/*
    proxy reference:
    치환된 bool은 1비트이므로 주소 값을 가지지 못함 -> 주소 단위 처리 불가
    -> 레퍼런스 처리 불가
    -> 레퍼런스 역할을 대신 해주는 객체를 중간에 둔다.
*/
namespace ft
{
    // 1워드의 비트 수를 나타내는 상수
    // 1워드: unsigned int
    // CHAR_BIT: 1바이트당 몇 비트인지 나타내는 상수
    static const size_t FT_WORD_BIT = CHAR_BIT * sizeof(unsigned int);

    class _Bit_reference
    {
      private:
        unsigned int *_p;
        unsigned int  mask; // 1비트만 켜짐, 이 비트 레퍼런스의 위치 나타냄

      public:
        _Bit_reference() : _p(0), mask(0) {}
        _Bit_reference(unsigned int *p, unsigned int m) : _p(p), mask(m) {}

        operator bool() const
        {
            // 0 혹은 mask 반환
            // 0 혹은 1로 정규화하기 위해 != 0
            return (*_p & mask) != 0;
        }
        _Bit_reference &operator=(bool x)
        {
            // 참이면 이 위치의 비트를 키고, 거짓이면 비트를 끈다
            if (x)
                *_p |= mask;
            else
                *_p &= ~mask;
            return *this;
        }
        _Bit_reference &operator=(const _Bit_reference &x)
        {
            // x의 비트 켜짐 유무를 따르고 자기 자신을 그대로 반환
            return *this = bool(x);
        }
        bool operator==(const _Bit_reference &x) const { return bool(*this) == bool(x); }
        bool operator<(const _Bit_reference &x) const
        {
            // 비교 연산자
            // this가 0이고 x가 1일때만 true
            return !bool(*this) && bool(x);
        }
        void flip()
        {
            // ^=: XOR
            // 이 위치에 해당하는 비트를 뒤집기
            *_p ^= mask;
        }
    };

    inline void swap(_Bit_reference x, _Bit_reference y)
    {
        bool tmp = x; // operator bool() 호출
        x = y;
        y = tmp;
    }

} // namespace ft

// =============================== Bit_iterator ================================

// 명세상 const iterator는 bool 타입
namespace ft
{
    struct _Bit_iterator_base : public ft::random_access_iterator<bool, ptrdiff_t>
    {
        unsigned int *_p;
        unsigned int  _offset; // 현재 워드 내에서 인덱스 [0, 31]

        typedef ptrdiff_t difference_type;

        _Bit_iterator_base() : _p(0), _offset(0) {}
        _Bit_iterator_base(unsigned int *p, unsigned int o) : _p(p), _offset(o) {}

        void _bump_up()
        {
            if (_offset == FT_WORD_BIT - 1)
            {
                _offset = 0;
                ++_p;
            }
            else
                ++_offset;
        }
        void _bump_down()
        {
            if (_offset == 0)
            {
                _offset = FT_WORD_BIT - 1;
                --_p;
            }
            else
                --_offset;
        }
        void _incr(difference_type i)
        {
            // FT_WORD_BIT가 unsigned라서 FT_WORD_BIT 포함 계산시 결과가 unsigned가 될 수 있음
            // 따라서 signed로 형변환하여 사용
            const int       W = (int)FT_WORD_BIT;
            difference_type n = i + (difference_type)_offset;
            _p += n / W;
            n %= W;

            if (n < 0)
            {
                n += W;
                --_p;
            }
            _offset = (unsigned int)n;
        }

        bool operator==(const _Bit_iterator_base &i) const
        {
            return _p == i._p && _offset == i._offset;
        }
        bool operator<(const _Bit_iterator_base &i) const
        {
            return _p < i._p || (_p == i._p && _offset < i._offset);
        }
        bool operator!=(const _Bit_iterator_base &i) const { return !(*this == i); }
        bool operator>(const _Bit_iterator_base &i) const { return i < *this; }
        bool operator<=(const _Bit_iterator_base &i) const { return !(i < *this); }
        bool operator>=(const _Bit_iterator_base &i) const { return !(*this < i); }
    };

    inline ptrdiff_t operator-(const _Bit_iterator_base &x, const _Bit_iterator_base &y)
    {
        if (x._p == 0 && y._p == 0)
            return 0;
        return (FT_WORD_BIT * (x._p - y._p)) + (x._offset - y._offset);
    }

    struct _Bit_iterator : public _Bit_iterator_base
    {
        typedef _Bit_reference  reference;
        typedef _Bit_reference *pointer;
        typedef _Bit_iterator   iterator;

        _Bit_iterator() : _Bit_iterator_base(0, 0) {}
        _Bit_iterator(unsigned int *p, unsigned int o) : _Bit_iterator_base(p, o) {}

        // bit reference 반환
        reference operator*() const { return reference(_p, 1U << _offset); }
        reference operator[](difference_type i) { return *(*this + i); }

        // 전위 연산자, 후위 연산자
        iterator &operator++()
        {
            _bump_up();
            return *this;
        }
        iterator operator++(int)
        {
            iterator tmp = *this;
            _bump_up();
            return tmp;
        }
        iterator &operator--()
        {
            _bump_down();
            return *this;
        }
        iterator operator--(int)
        {
            iterator tmp = *this;
            _bump_down();
            return tmp;
        }
        iterator &operator+=(difference_type i)
        {
            _incr(i);
            return *this;
        }
        iterator &operator-=(difference_type i)
        {
            *this += -i;
            return *this;
        }
        iterator operator+(difference_type i) const
        {
            iterator tmp = *this;
            return tmp += i;
        }
        iterator operator-(difference_type i) const
        {
            iterator tmp = *this;
            return tmp -= i;
        }
    };

    inline _Bit_iterator operator+(ptrdiff_t n, const _Bit_iterator &x) { return x + n; }

    struct _Bit_const_iterator : public _Bit_iterator_base
    {
        // Bit_reference 타입이 아닌 bool 타입
        // operator*()가 반환하는 건 실제로 bool 값(복사본)
        typedef bool                reference;
        typedef bool                const_reference;
        typedef const bool         *pointer;
        typedef _Bit_const_iterator const_iterator;

        _Bit_const_iterator() : _Bit_iterator_base(0, 0) {}
        _Bit_const_iterator(unsigned int *x, unsigned int y) : _Bit_iterator_base(x, y) {}
        _Bit_const_iterator(const _Bit_iterator &x) : _Bit_iterator_base(x._p, x._offset) {}

        // bit reference 반환
        const_reference operator*() const { return bool(_Bit_reference(_p, 1U << _offset)); }
        const_reference operator[](difference_type i) { return *(*this + i); }

        // 전위 연산자, 후위 연산자
        const_iterator &operator++()
        {
            _bump_up();
            return *this;
        }
        const_iterator operator++(int)
        {
            const_iterator tmp = *this;
            _bump_up();
            return tmp;
        }
        const_iterator &operator--()
        {
            _bump_down();
            return *this;
        }
        const_iterator operator--(int)
        {
            const_iterator tmp = *this;
            _bump_down();
            return tmp;
        }
        const_iterator &operator+=(difference_type i)
        {
            _incr(i);
            return *this;
        }
        const_iterator &operator-=(difference_type i)
        {
            *this += -i;
            return *this;
        }
        const_iterator operator+(difference_type i) const
        {
            const_iterator tmp = *this;
            return tmp += i;
        }
        const_iterator operator-(difference_type i) const
        {
            const_iterator tmp = *this;
            return tmp -= i;
        }
    };

    inline _Bit_const_iterator operator+(ptrdiff_t n, const _Bit_const_iterator &x)
    {
        return x + n;
    }
} // namespace ft

// =============================== Bvector =====================================

namespace ft
{
    template <class _Alloc, bool _instanceless>
    class _Bvector_alloc_base;

    // Base class for ordinary allocators.
    template <class _Alloc>
    class _Bvector_alloc_base<_Alloc, false>
    {
      public:
        typedef typename ft::_Alloc_traits<bool, _Alloc>::allocator_type allocator_type;
        allocator_type get_allocator() const { return _alloc; }

        _Bvector_alloc_base(const allocator_type &a) : _alloc(a) {}

      protected:
        allocator_type _alloc;
    };

    // Specialization for instanceless allocators
    // empty class
    template <class _Alloc>
    class _Bvector_alloc_base<_Alloc, true>
    {
      public:
        typedef typename ft::_Alloc_traits<bool, _Alloc>::allocator_type allocator_type;
        allocator_type get_allocator() const { return allocator_type(); }

        _Bvector_alloc_base(const allocator_type &) {}

      protected:
    };

    template <class _Alloc>
    class _Bvector_base
        : public _Bvector_alloc_base<_Alloc, ft::_Alloc_traits<bool, _Alloc>::_instanceless>
    {
        typedef _Bvector_alloc_base<_Alloc, ft::_Alloc_traits<bool, _Alloc>::_instanceless> _Base;

      public:
        typedef typename _Base::allocator_type allocator_type;

        _Bvector_base(const allocator_type &a) : _Base(a), _start(), _finish(), _end_of_storage(0)
        {
        }
        ~_Bvector_base() { _deallocate(); }

      protected:
        typedef typename allocator_type::template rebind<unsigned int>::other word_allocator_type;

        _Bit_iterator _start;
        _Bit_iterator _finish;
        unsigned int *_end_of_storage;

        unsigned int *_bit_alloc(size_t n)
        {
            // n: 할당할 비트 수
            if (n == 0)
                return 0;
            // 무조건 1워드는 할당
            word_allocator_type wa(_Base::get_allocator());
            return wa.allocate((n + FT_WORD_BIT - 1) / FT_WORD_BIT);
        }
        void _deallocate()
        {
            if (!_start._p || !_end_of_storage)
                return;
            const size_t words = (size_t)(_end_of_storage - _start._p);
            if (words == 0)
                return;
            word_allocator_type wa(_Base::get_allocator());
            wa.deallocate(_start._p, words);
        }
    };
} // namespace ft

// =============================== vector<bool> ================================

namespace ft
{
    template <class _Alloc>
    class vector<bool, _Alloc> : public _Bvector_base<_Alloc>
    {
      public:
        typedef bool      value_type;
        typedef size_t    size_type;
        typedef ptrdiff_t difference_type;

        // 명세상 const_reference는 그냥 bool 타입
        typedef _Bit_reference reference;
        typedef bool           const_reference;

        typedef _Bit_reference *pointer;
        typedef const bool     *const_pointer;

        typedef _Bit_iterator       iterator;
        typedef _Bit_const_iterator const_iterator;

        typedef ft::reverse_iterator<const_iterator> const_reverse_iterator;
        typedef ft::reverse_iterator<iterator>       reverse_iterator;

        typedef typename _Bvector_base<_Alloc>::allocator_type allocator_type;
        allocator_type get_allocator() const { return _Bvector_base<_Alloc>::get_allocator(); }

      protected:
        using _Bvector_base<_Alloc>::_bit_alloc;
        using _Bvector_base<_Alloc>::_deallocate;
        using _Bvector_base<_Alloc>::_start;
        using _Bvector_base<_Alloc>::_finish;
        using _Bvector_base<_Alloc>::_end_of_storage;

      protected:
        void _range_check(size_type n) const
        {
            if (n >= this->size())
                ft::throw_out_of_range_index("vector<bool>", "_range_check", n, this->size());
        }

      public:
        // iterator
        iterator       begin() { return _start; }
        const_iterator begin() const { return _start; }
        iterator       end() { return _finish; }
        const_iterator end() const { return _finish; }

        // reverse-iterator
        reverse_iterator       rbegin() { return reverse_iterator(end()); }
        const_reverse_iterator rbegin() const { return const_reverse_iterator(end()); }
        reverse_iterator       rend() { return reverse_iterator(begin()); }
        const_reverse_iterator rend() const { return const_reverse_iterator(begin()); }

        // -------------------- Capacity -------------------- //
        size_type size() const { return size_type(end() - begin()); }
        size_type max_size() const { return size_type(-1); }
        size_type capacity() const
        {
            return _start._p ? size_type(const_iterator(_end_of_storage, 0) - begin()) : 0;
        }
        bool empty() const { return begin() == end(); }
        void reserve(size_type n)
        {
            if (capacity() < n)
            {
                unsigned int *q = _bit_alloc(n);
                _finish = ft::copy(begin(), end(), iterator(q, 0));
                _deallocate();
                _start = iterator(q, 0);
                _end_of_storage = q + (n + FT_WORD_BIT - 1) / FT_WORD_BIT;
            }
        }

        // -------------------- Element access -------------------- //
        // operator[] : 빠른 버전 (범위 밖이면 UB)
        // at() : 안전 버전 (범위 밖이면 예외 던짐)
        reference       operator[](size_type n) { return *(begin() + n); }
        const_reference operator[](size_type n) const { return *(begin() + n); }
        reference       at(size_type n)
        {
            _range_check(n);
            return (*this)[n];
        }
        const_reference at(size_type n) const
        {
            _range_check(n);
            return (*this)[n];
        }

        // -------------------- Constructors -------------------- //
        // 인자가 하나인 생성자들에 대해서는 implicit conversion을 막아둔다

        explicit vector(const allocator_type &a = allocator_type()) : _Bvector_base<_Alloc>(a) {}

        vector(size_type n, bool value, const allocator_type &a = allocator_type())
            : _Bvector_base<_Alloc>(a)
        {
            _initialize(n);
            // ~0: 모든 비트가 1로 채워져있는 수
            ft::fill(_start._p, _end_of_storage, value ? ~0 : 0);
        }

        explicit vector(size_type n) : _Bvector_base<_Alloc>(allocator_type())
        {
            _initialize(n);
            ft::fill(_start._p, _end_of_storage, 0);
        }

        vector(const vector &x) : _Bvector_base<_Alloc>(x.get_allocator())
        {
            _initialize(x.size());
            ft::copy(x.begin(), x.end(), _start);
        }

        // 위 생성자 시그니처랑 겹쳐보이는데, 이건 보험용임 (모호한 타입 처리)
        template <class _InputIter>
        vector(_InputIter first, _InputIter last, const allocator_type &a = allocator_type())
            : _Bvector_base<_Alloc>(a)
        {
            typedef ft::is_integral<_InputIter> _is_integral;
            _initialize_dispatch(first, last, _is_integral());
        }

        template <class _Integer>
        void _initialize_dispatch(_Integer n, _Integer x, true_type)
        {
            // n: size, x: value
            _initialize(n);
            ft::fill(_start._p, _end_of_storage, x ? ~0 : 0);
        }

        template <class _InputIter>
        void _initialize_dispatch(_InputIter first, _InputIter last, false_type)
        {
            typedef typename ft::iterator_traits<_InputIter>::iterator_category _cat;
            _initialize_range(first, last, _cat());
        }

        ~vector() {}

        vector &operator=(const vector &x)
        {
            if (&x == this)
                return *this;
            if (x.size() > capacity())
            {
                _deallocate();
                _initialize(x.size());
            }
            ft::copy(x.begin(), x.end(), begin());
            _finish = begin() + difference_type(x.size());
            return *this;
        }

        // assign(): Replaces the contents of the container
        // 같은 값을 여러 개 채우는 타입과 구간 복사 타입이 있음

        /*
            _fill_assign: 같은 값을 여러 개 채울 때
            _assign_aux: 구간 복사 채우기
        */

        void _fill_assign(size_t n, bool x)
        {
            if (n > size()) // 현재 원소보다 많은 양
            {
                // 기존 원소들에 값을 복사한 후 range_insert
                ft::fill(_start._p, _end_of_storage, x ? ~0 : 0);
                insert(end(), n - size(), x);
            }
            else // 현재 원소보다 적은 양
            {
                // 초과하는 개수만큼 range_erase 후 기존 원소에 값 복사
                erase(begin() + n, end());
                ft::fill(_start._p, _end_of_storage, x ? ~0 : 0);
            }
        }

        void assign(size_t n, bool x) { _fill_assign(n, x); }

        // 한 번만 진행 가능한 iterator: 하나씩 처리
        template <class _InputIter>
        void _assign_aux(_InputIter first, _InputIter last, input_iterator_tag)
        {
            iterator cur = begin();
            for (; first != last && cur != end(); ++cur, ++first)
                *cur = *first;

            // 새 범위에 맞춰 나머지 처리
            // 초과분 만큼 지우거나, 부족분 만큼 하나씩 삽입
            if (first == last)
                erase(cur, end());
            else
                insert(end(), first, last);
        }

        // 여러 번 진행 가능한 iterator: bulk 처리
        template <class _ForwardIterator>
        void _assign_aux(_ForwardIterator first, _ForwardIterator last, forward_iterator_tag)
        {
            size_type len = distance(first, last);

            if (len < size())
                // copy 후 초과분 만큼 range_erase
                erase(ft::copy(first, last, begin()), end());
            else
            {
                // 현재까지 생성되어있는 원소까지만 copy 후,
                // 부족분만큼 range_insert
                _ForwardIterator idx = first;
                // forward iterator는 random access가 보장되어 있지 않음
                std::advance(idx, size());
                ft::copy(first, idx, begin());
                insert(end(), idx, last);
            }
        }

        template <class _Integer>
        void _assign_dispatch(_Integer n, _Integer val, true_type)
        {
            _fill_assign((size_t)n, (bool)val);
        }

        template <class _InputIter>
        void _assign_dispatch(_InputIter first, _InputIter last, false_type)
        {
            typedef typename ft::iterator_traits<_InputIter>::iterator_category _cat;
            _assign_aux(first, last, _cat());
        }

        template <class _InputIter>
        void assign(_InputIter first, _InputIter last)
        {
            typedef ft::is_integral<_InputIter> _is_integral;
            _assign_dispatch(first, last, _is_integral());
        }

        // -------------------- Element access -------------------- //
        reference       front() { return *begin(); }
        const_reference front() const { return *begin(); }
        reference       back() { return *(end() - 1); }
        const_reference back() const { return *(end() - 1); }

        // -------------------- Modifiers -------------------- //
        void _debug_invariant() const
        {
            if (!_start._p)
            {
                // empty state
                assert(_end_of_storage == 0);
                assert(_finish._p == 0 && _finish._offset == 0);
                return;
            }

            assert(_end_of_storage != 0);
            assert(_start._p <= _finish._p);
            assert(_finish._p <= _end_of_storage);
            assert(_finish._offset < FT_WORD_BIT);

            // if finish points to end_of_storage, offset must be 0 (word boundary)
            if (_finish._p == _end_of_storage)
                assert(_finish._offset == 0);
        }

        void push_back(bool x)
        {
            if (_finish._p != _end_of_storage)
                *_finish++ = x;
            else
                _insert_aux(end(), x);
            _debug_invariant();
        }
        void pop_back() { --_finish; }

        void swap(vector<bool, _Alloc> &x)
        {
            ft::swap(_start, x._start);
            ft::swap(_finish, x._finish);
            ft::swap(_end_of_storage, x._end_of_storage);
        };

        void _fill_insert(iterator position, size_type n, bool x)
        {
            if (n == 0)
                return;
            if (capacity() >= size() + n)
            {
                ft::copy_backward(position, end(), _finish + difference_type(n));
                ft::fill(position, position + difference_type(n), x);
                _finish += difference_type(n);
            }
            else
            {
                size_type     len = size() + std::max(size(), n);
                unsigned int *q = _bit_alloc(len);
                iterator      i = ft::copy(begin(), position, iterator(q, 0));
                ft::fill_n(i, n, x);
                _finish = ft::copy(position, end(), i + difference_type(n));
                _deallocate();
                _end_of_storage = q + (len + FT_WORD_BIT - 1) / FT_WORD_BIT;
                _start = iterator(q, 0);
            }
        }

        iterator insert(iterator position, bool x = bool())
        {
            difference_type n = position - begin();
            // 공간이 충분하고 삽입 위치가 맨 마지막이라면 copy 필요없음
            if (_finish._p != _end_of_storage && position == end())
                *_finish++ = x;
            else
                _insert_aux(position, x);
            return begin() + n;
        }

        void _insert_aux(iterator position, bool x)
        {
            if (_finish._p != _end_of_storage)
            {
                ft::copy_backward(position, _finish, _finish + 1);
                *position = x;
                ++_finish;
            }
            else
            {
                size_type     len = size() ? 2 * size() : FT_WORD_BIT;
                unsigned int *q = _bit_alloc(len);
                iterator      i = ft::copy(begin(), position, iterator(q, 0));
                *i++ = x;
                _finish = copy(position, end(), i);
                _deallocate();
                _end_of_storage = q + (len + FT_WORD_BIT - 1) / FT_WORD_BIT;
                _start = iterator(q, 0);
            }
        }

        void insert(iterator position, size_type n, bool x) { _fill_insert(position, n, x); }

        template <class _Integer>
        void _insert_dispatch(iterator pos, _Integer n, _Integer x, true_type)
        {
            // 같은 값 삽입
            _fill_insert(pos, n, x);
        }

        template <class _InputIter>
        void _insert_dispatch(iterator pos, _InputIter first, _InputIter last, false_type)
        {
            typedef typename iterator_traits<_InputIter>::iterator_category _cat;
            // 구간 복사 삽입
            _insert_range(pos, first, last, _cat());
        }

        template <class _InputIter>
        void insert(iterator position, _InputIter first, _InputIter last)
        {
            typedef ft::is_integral<_InputIter> _is_integral;
            _insert_dispatch(position, first, last, _is_integral());
        }

        iterator erase(iterator position)
        {
            // 지우는 위치가 마지막이면 copy할 필요 없음
            if (position + 1 != end())
                ft::copy(position + 1, end(), position);
            --_finish;
            return position;
        }
        iterator erase(iterator first, iterator last)
        {
            _finish = ft::copy(last, end(), first);
            return first;
        }

        void resize(size_type new_size, bool x = bool())
        {
            if (new_size < size())
                erase(begin() + difference_type(new_size), end());
            else
                insert(end(), new_size - size(), x);
        }

        void flip()
        {
            // 벡터가 가진 모든 워드를 뒤집는다
            for (unsigned int *p = _start._p; p != _end_of_storage; ++p)
                *p = ~*p;
        }

        void clear() { erase(begin(), end()); }

      private:
        // n비트를 갖도록 메모리 할당 (값은 채우지 않음)
        void _initialize(size_type n)
        {
            if (n == 0)
            {
                _start = iterator();
                _finish = iterator();
                _end_of_storage = 0;
                return;
            }
            unsigned int *q = _bit_alloc(n);
            _end_of_storage = q + (n + FT_WORD_BIT - 1) / FT_WORD_BIT;
            _start = iterator(q, 0);
            _finish = _start + difference_type(n);
        }

        // 한 번만 진행 가능한 iterator: 하나씩 처리
        template <class _InputIter>
        void _initialize_range(_InputIter first, _InputIter last, input_iterator_tag)
        {
            _start = iterator();
            _finish = iterator();
            _end_of_storage = 0;
            for (; first != last; ++first)
                push_back(*first);
        }

        // 여러 번 진행 가능한 iterator: bulk 처리
        template <class _ForwardIterator>
        void _initialize_range(_ForwardIterator first, _ForwardIterator last, forward_iterator_tag)
        {
            size_type n = ft::distance(first, last);
            _initialize(n);
            ft::copy(first, last, _start);
        }

        // 한 번만 진행 가능한 iterator: 하나씩 처리
        template <class _InputIter>
        void _insert_range(iterator pos, _InputIter first, _InputIter last, input_iterator_tag)
        {
            for (; first != last; ++first)
            {
                pos = insert(pos, *first);
                ++pos;
            }
        }

        // 여러 번 진행 가능한 iterator: bulk 처리
        template <class _ForwardIterator>
        void _insert_range(iterator position, _ForwardIterator first, _ForwardIterator last,
                           forward_iterator_tag)
        {
            if (first != last)
            {
                size_type n = ft::distance(first, last);

                if (capacity() >= size() + n) // 공간 충분함
                {
                    ft::copy_backward(position, end(), _finish + difference_type(n));
                    ft::copy(first, last, position);
                    _finish += difference_type(n);
                }
                else
                {
                    // 새로 할당
                    size_type     len = size() + std::max(size(), n); // 최소 2배 이상으로 키움
                    unsigned int *q = _bit_alloc(len);
                    iterator      i;

                    // [start, pos)
                    i = ft::copy(begin(), position, iterator(q, 0));
                    // [pos, pos + n)
                    i = ft::copy(first, last, i);
                    // [pos + n, finish)
                    _finish = ft::copy(position, end(), i);
                    _deallocate();
                    _end_of_storage = q + (len + FT_WORD_BIT - 1) / FT_WORD_BIT;
                    _start = iterator(q, 0);
                }
            }
        }
    };
} // namespace ft

#endif

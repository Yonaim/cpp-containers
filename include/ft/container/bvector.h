/**
 * SUMMARY: partial specialization for vector<bool>
 * https://en.cppreference.com/w/cpp/container/vector_bool.html
 *
 * : bool 타입 객체를 연속적으로 모아두는 대신 bool 타입 객체 하나를 1비트로 치환하여 저장 *
 */

#ifndef FT_BVECTOR_H
#define FT_BVECTOR_H

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
    static const size_t WORD_BIT = CHAR_BIT * sizeof(unsigned int);

    class _Bit_reference
    {
      private:
        unsigned int *_p;
        unsigned int  _mask; // 1비트만 켜짐, 이 비트 레퍼런스의 위치 나타냄

      public:
        _Bit_reference() : _p(0), _mask(0) {}
        _Bit_reference(unsigned int *p, unsigned int m) : _p(p), _mask(m) {}

        operator bool() const
        {
            // 0 혹은 _mask 반환
            // 0 혹은 1로 정규화하기 위해 != 0
            return (*_p & _mask) != 0;
        }
        _Bit_reference &operator=(bool x)
        {
            // 참이면 이 위치의 비트를 키고, 거짓이면 비트를 끈다
            if (x)
                *_p |= _mask;
            else
                *_p &= ~_mask;
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
            *_p ^= _mask;
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

/*
    명세상
    - iterator는
    - const iterator는 bool 타입

*/
namespace ft
{
    struct _Bit_iterator_base
    {
        unsigned int *_p;
        unsigned int  _offset; // 현재 워드 내에서 인덱스 [0, 31]

        typedef ptrdiff_t difference_type;

        _Bit_iterator_base(unsigned int *x, unsigned int y) : _p(x), _offset(y) {}

        void _bump_up()
        {
            ++_offset;
            if (_offset == WORD_BIT)
            {
                _offset = 0;
                ++_p;
            }
        }
        void _bump_down()
        {
            --_offset;
            if (_offset == 0)
            {
                _offset = WORD_BIT - 1;
                --_p;
            }
        }
        void _incr(ptrdiff_t i)
        {
            difference_type n = i + _offset; // signed
            _p += n / WORD_BIT;
            n += n % WORD_BIT;
            if (n < 0)
            {
                _offset = (unsigned int)(n + WORD_BIT);
                --_p;
            }
            else
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
        return (WORD_BIT * (x._p - y._p)) + (x._offset - y._offset);
    }

    struct _Bit_iterator : public _Bit_iterator_base
    {
        typedef _Bit_reference  reference;
        typedef _Bit_reference *pointer;
        typedef _Bit_iterator   iterator;

        _Bit_iterator() : _Bit_iterator_base(0, 0) {}
        _Bit_iterator(unsigned int *x, unsigned int y) : _Bit_iterator_base(x, y) {}

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
    template <class _Tp>
    struct is_instanceless
    {
        static const bool value = ft::is_empty<_Tp>::value;
    };

    template <class _Alloc, bool _instanceless>
    class _Bvector_alloc_base;

    // Base class for ordinary allocators.
    template <class _Alloc>
    class _Bvector_alloc_base<_Alloc, false>
    {
      protected:
        _Alloc alloc;

      public:
        _Alloc get_allocator() { return alloc; }
    };

    // Specialization for instanceless allocators
    // empty class
    template <class _Alloc>
    class _Bvector_alloc_base<_Alloc, true>
    {
      public:
        _Alloc get_allocator() { return _Alloc(); }
    };

    template <class _Alloc, is_instanceless<_Alloc>::value>
    class _Bvector_base
    {
    };

} // namespace ft

// =============================== vector<bool> ================================

namespace ft
{
    template <class _Alloc>
    class vector<bool, _Alloc> : _Bvector_base<_Alloc>
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

        typedef reverse_iterator<const_iterator> const_reverse_iterator;
        typedef reverse_iterator<iterator>       reverse_iterator;

        typedef typename _Bvector_base<_Alloc>::allocator_type allocator_type;
        allocator_type get_allocator() const { return _Bvector_base<_Alloc>::get_allocator(); }
    };
} // namespace ft

#endif

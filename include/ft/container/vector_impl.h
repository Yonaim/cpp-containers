#ifndef FT_VECTOR_3PTR_IMPL_H
#define FT_VECTOR_3PTR_IMPL_H

// #include "vector_3ptr.h"
#include <iostream>
#include <sstream>
#include "ft_memory.h"
#include "utility.h"

/*
    - base 클래스가 RAII로 메모리 관리를 해주지만, 소멸자 호출시 자동 정리되는 것은
        현재 멤버 변수 (_start, _end_of_storage)에 할당된 것으로 잡혀있는 메모리 영역에 대해서만임
    - 따라서 현재 처리하는 메모리가 멤버 변수에 저장되어 있지 않다면 exception-safety 보장을 위한
        예외 처리는 여전히 필요함
*/

namespace ft
{
    // -------------------- Constructors -------------------- //

    template <class _Tp, class _Alloc>
    vector<_Tp, _Alloc>::vector(const allocator_type &a) : _Base(a)
    {
    }

    // Fill constructor (with specified value)
    template <class _Tp, class _Alloc>
    vector<_Tp, _Alloc>::vector(size_type n, const _Tp &value, const allocator_type &a)
        : _Base(n, a)
    {

        _finish = ft::uninitialized_fill_n(_start, n, value);
    }

    template <class _Tp, class _Alloc>
    vector<_Tp, _Alloc>::vector(size_type n) : _Base(n, allocator_type())
    {
        _finish = ft::uninitialized_fill_n(_start, n, _Tp());
    }

    // Copy constructor
    template <class _Tp, class _Alloc>
    vector<_Tp, _Alloc>::vector(const vector<_Tp, _Alloc> &x) : _Base(x.size(), x.get_allocator())
    {
        _finish = ft::uninitialized_copy(x.begin(), x.end(), this->_start);
    }

    /*
        Fill constructor와 함수 원형이 혼동될 수 있는 현상을 해결하는 두 가지 방법이 있다.
        1) 함수 내부에서 is_integer 이용하여 분기 처리
        2) enable_if 사용하여 integer인 경우에 함수 후보 자체를 제외시킴

        2가 좀더 깔끔한 방법이다.
    */
    /*
        iterator_category와 같은 것을 typedef로 만들려고 시도할 때, '인스턴스화하는 과정에서' 에러.
        SFINAE로 조용히 넘어가는게 아니라 하드 에러가 터진다.
        하드 에러가 터지는 즉시 컴파일이 중단되므로 우아한 해결책이 아님.
        (enable_if는 인스턴스는 생성되되 멤버만 없을 뿐으로, SFINAE로 취급)

        그래서 타협안:
        정수만 배제해서 (n, value)와 충돌만 막고 진짜 iterator 여부는 런타임 에러가 나게 둠
    */
    template <class _Tp, class _Alloc>
    template <class _InputIt>
    vector<_Tp, _Alloc>::vector(
        _InputIt first, _InputIt last, const allocator_type &a,
        typename ft::enable_if<!ft::is_integral<_InputIt>::value, void>::type *)
        : _Base(a)
    {
        _range_initialize(first, last, typename iterator_traits<_InputIt>::iterator_category());
    }

    // Destructor
    template <class _Tp, class _Alloc>
    vector<_Tp, _Alloc>::~vector()
    {
        destroy_range(_start, _finish);
    }

    // -------------------- Capacity -------------------- //

    template <class _Tp, class _Alloc>
    template <class _ForwardIterator>
    typename vector<_Tp, _Alloc>::pointer
    vector<_Tp, _Alloc>::_allocate_and_copy(size_type n, _ForwardIterator first,
                                            _ForwardIterator last)
    {
        pointer mem = _allocate(n);
        try
        {
            ft::uninitialized_copy(first, last, mem);
            return mem;
        }
        catch (...)
        {
            _deallocate(mem, n);
            throw;
        }
    }

    // Increase the capacity of the vector
    // If new_cap is greater than the current capacity(), new storage is allocated,
    // otherwise the function does nothing.
    /*
        Trivially Copyable(자명하게 복사 가능한)
        -객체를 메모리 블록 단위로 memcpy / memmove 같은 저수준 복사 함수로 옮겨도 안전하게 쓸 수
       있는 타입
        - POD(Plain Old Data)는 trivially copyable
        - 모던 c++에 is_trivially_copyable 함수 존재
    */
    template <class _Tp, class _Alloc>
    void vector<_Tp, _Alloc>::reserve(size_type new_cap)
    {
        if (new_cap > capacity())
        {
            size_t  old_size = size();
            pointer new_start = _allocate_and_copy(new_cap, _start, _finish);
            destroy_range(_start, _finish);
            _deallocate(_start, capacity());
            _start = new_start;
            _finish = _start + old_size;
            _end_of_storage = _start + new_cap;
        }
    }

    // -------------------- member operator -------------------- //
    // Copy assignment operator
    template <class _Tp, class _Alloc>
    vector<_Tp, _Alloc> &vector<_Tp, _Alloc>::operator=(const vector<_Tp, _Alloc> &x)
    {
        /*
        - 공간이 불충분한 경우: 재할당 및 객체 생성
        - 공간은 충분
            - 활성화된 원소 개수가 과다한 경우: 원본 객체 복사 및 초과분 destory
            - 활성화된 원소 개수가 적은 경우: 부족분 ft::uninitialized_copy 수행
        */
        if (&x != this)
        {
            size_t x_size = x.size();
            if (capacity() < x_size)
            {
                pointer new_start = _allocate_and_copy(x_size, x.begin(), x.end());
                destroy_range(x.begin(), x.end());
                _deallocate(_start, capacity());
                _start = new_start;
                _finish = new_start + x_size;
                _end_of_storage = _finish;
            }
            else if (size() > x_size)
            {
                copy(x.begin(), x.end(), begin());
                destroy_range(begin() + (size() - x_size), end());
                _finish = _start + x_size;
            }
            else
            {
                ft::uninitialized_copy(x.begin() + size(), x.end(), begin() + size());
                copy(x.begin(), x.begin() + size(), begin());
                _finish = _start + x_size;
            }
        }
        return *this;
    }

    // -------------------- Modifiers -------------------- //

    template <class _Tp, class _Alloc>
    void vector<_Tp, _Alloc>::push_back(const _Tp &x)
    {
        // 맨 뒤에 삽입 -> 공간 충분할시 원소 미루기 (copy) 필요없음
        if (_finish != _end_of_storage)
        {
            _construct(_finish, x);
            ++_finish;
        }
        else
            _insert_aux(end(), x);
    }

    template <class _Tp, class _Alloc>
    void vector<_Tp, _Alloc>::push_back()
    {
        push_back(_Tp());
    }

    template <class _Tp, class _Alloc>
    void vector<_Tp, _Alloc>::pop_back()
    {
        --_finish;
        _destroy(_finish);
    }

    template <class _Tp, class _Alloc>
    void vector<_Tp, _Alloc>::swap(vector<_Tp, _Alloc> &x)
    {
        ft::swap(_start, x._start);
        ft::swap(_finish, x._finish);
        ft::swap(_end_of_storage, x._end_of_storage);
    }

    template <class _Tp, class _Alloc>
    typename vector<_Tp, _Alloc>::iterator vector<_Tp, _Alloc>::insert(iterator   position,
                                                                       const _Tp &x)
    {
        // 공간이 충분하고 삽입 위치가 맨 마지막일시 copy할 필요 없음
        if (_finish != _end_of_storage && position == end())
        {
            _construct(_finish, x);
            ++_finish;
        }
        else
            _insert_aux(position, x);
        return position;
    }

    template <class _Tp, class _Alloc>
    typename vector<_Tp, _Alloc>::iterator vector<_Tp, _Alloc>::insert(iterator position)
    {
        return insert(position, _Tp());
    }

    // range insert
    /*
        iterator의 tag에 따라 분기 처리
        (1) Input iterator
            - distance 계산 & multi-pass 둘다 불가능
            - 하나씩 insert() 해야함.
            - insert()는 O(n)이고 이것을 n번 실행, 즉 O(n^2)
        (2) Forward & Bidirectional iterator
            - distance 계산 O(n)에 가능
            - 이후 O(n) 처리 가능
        (3) Random Access
            - distance 계산 O(1)에 가능
            - 이후 O(n) 처리 가능
    */
    template <class _Tp, class _Alloc>
    template <class _InputIt>
    void vector<_Tp, _Alloc>::insert(
        iterator pos, _InputIt first, _InputIt last,
        typename ft::enable_if<!ft::is_integral<_InputIt>::value, void>::type *)
    {
        // 한번만 읽을 수 있는 iterator인 경우: 하나씩 insert
        // 여러번 읽을 수 있는 iterator인 경우: ft::distance를 얻고 일괄 처리
        // tag dispatch
        _insert_dispatch(pos, first, last, typename iterator_traits<_InputIt>::iterator_category());
    }

    // 한번만 읽을 수 있는 iterator인 경우: 하나씩 insert
    template <class _Tp, class _Alloc>
    template <class _InputIt>
    void vector<_Tp, _Alloc>::_insert_dispatch(iterator pos, _InputIt first, _InputIt last,
                                               ft::input_iterator_tag)
    {
        for (; first != last; ++first, ++pos)
            _insert_aux(pos, *first);
    }

    // 여러번 읽을 수 있는 iterator인 경우: ft::distance를 얻고 일괄 처리
    template <class _Tp, class _Alloc>
    template <class _InputIt>
    void vector<_Tp, _Alloc>::_insert_dispatch(iterator pos, _InputIt first, _InputIt last,
                                               ft::forward_iterator_tag)
    {
        size_t old_size = size();

        difference_type n = ft::distance(first, last);
        if (capacity() > old_size + n) // 공간 충분: pos 이후만 조작
        {
            // 기존 원소 뒤로 옮기기 (init도 겸함)
            // input을 copy
            ft::uninitialized_copy_backward(pos, end(), end() + n);
            copy(first, last, pos);
            _finish += n;
        }
        else // 공간 부족: 전체 재할당 및 복사
        {
            // 재할당하므로 복사시 겹침 걱정 안 해도 됨
            size_t idx = ft::distance(begin(), pos);

            // [start, pos)
            pointer new_start = _allocate_and_copy(old_size + n, _start, pos);
            // [pos, pos + n) : 주어진 값 copy
            for (difference_type i = 0; i < n; ++i)
                *(new_start + idx + i) = *(first + i);
            // [pos + n, finish) : init + copy
            ft::uninitialized_copy(pos, end(), new_start + n);

            destroy_range(_start, _finish);
            _deallocate(_start, capacity());

            _start = new_start;
            _finish = _start + old_size + n;
            _end_of_storage = _finish;
        }
    }

    // range insert
    template <class _Tp, class _Alloc>
    void vector<_Tp, _Alloc>::insert(iterator pos, size_type n, const _Tp &x)
    {
        size_t old_size = size();
        if (capacity() > old_size + n) // 공간 충분: pos 이후만 조작
        {
            // 기존 원소 뒤로 옮기기 (init도 겸함)
            // input을 copy
            ft::uninitialized_copy_backward(pos, end(), pos + n);
            for (size_t i = 0; i < n; ++i)
                *(pos + i) = x;
            _finish += n;
        }
        else // 공간 부족: 전체 재할당 및 복사
        {

            // 재할당하므로 복사시 겹침 걱정 안 해도 됨
            size_t idx = ft::distance(begin(), pos);

            // [start, pos)
            pointer new_start = _allocate_and_copy(old_size + n, _start, pos);
            // [pos, pos + n) : 주어진 값 copy
            for (size_t i = 0; i < n; ++i)
                *(new_start + idx + i) = x;
            // [pos + n, finish) : init + copy
            ft::uninitialized_copy(pos, end(), new_start + n);

            destroy_range(_start, _finish);
            _deallocate(_start, capacity());

            _start = new_start;
            _finish = _start + old_size + n;
            _end_of_storage = _finish;
        }
    }

    template <class _Tp, class _Alloc>
    typename vector<_Tp, _Alloc>::iterator vector<_Tp, _Alloc>::erase(iterator position)
    {
        // 맨 마지막이 아닌 중간 원소를 지운다면 정방향 copy 필요 (앞으로 한 칸 당기기)
        if (position != (end() - 1))
            copy(position + 1, end(), position);
        --_finish;
        _destroy(_finish);
        return position;
    }

    template <class _Tp, class _Alloc>
    typename vector<_Tp, _Alloc>::iterator vector<_Tp, _Alloc>::erase(iterator first, iterator last)
    {
        // 1. 지우는 크기만큼 앞으로 당기기 (정방향 copy)
        // 2. 지우는 크기만큼 끝에서 destroy
        difference_type n = ft::distance(first, last);
        copy(last, end(), first);
        destroy_range(end() - n, end());
        _finish -= n;
        return first;
    }

    template <class _Tp, class _Alloc>
    void vector<_Tp, _Alloc>::resize(size_type new_size, const _Tp &x)
    {
        size_t old_size = size();
        if (new_size > old_size)
        {
            // new_size > size: 부족한 만큼 값 x 삽입
            size_t add = new_size - old_size;
            if (capacity() < new_size)
                reserve(new_size);
            ft::uninitialized_fill_n(begin() + old_size, add, x);
        }
        if (new_size < old_size)
        {
            // new_size < size: 넘치는 만큼 destroy
            size_t del = old_size - new_size;
            destroy_range(end() - del, end());
        }
        _finish = _start + new_size;
    }

    /* ------------------ Internal private methods ------------------  */

    template <class _Tp, class _Alloc>
    void vector<_Tp, _Alloc>::_range_check(size_type n) const
    {
        if (n >= this->size())
            ft::throw_out_of_range_index("vector", "_range_check", n, this->size());
    }

    // input iterator: 한번만 읽기 가능, 단방향 (++it만 됨)
    template <class _Tp, class _Alloc>
    template <class _InputIt>
    void vector<_Tp, _Alloc>::_range_initialize(_InputIt first, _InputIt last, input_iterator_tag)
    {
        for (; first != last; ++first)
            push_back(*first);
    }

    // forward iterator: 여러번 읽기 가능, 단방향 (++it만 됨)
    template <class _Tp, class _Alloc>
    template <class _InputIt>
    void vector<_Tp, _Alloc>::_range_initialize(_InputIt first, _InputIt last, forward_iterator_tag)
    {
        difference_type n = ft::distance(first, last);
        _start = _allocate(n);
        _end_of_storage = _start + n;
        _finish = ft::uninitialized_copy(first, last, _start);
    }

    static inline size_t _next_capacity(size_t old_cap) { return old_cap == 0 ? 1 : old_cap * 2; }

    /*
        미리 destroy하고 reverse 이터레이터를 인자로 uninitialized_copy() 호출하는 방식은 안되는
       이유???
        -> exception-safety 만족을 위해서는 미리 destroy를 하면 안된다.
            (destroy는 되돌릴 수 없는 연산!)

        가능하면 assignment(대입)을 한다.
        - copy constructor와 copy assignment는 전혀 다름
        - assignment시, 객체를 파괴/새로 생성하지 않고 현재 객체의 값만 교체해주면 됨
        - 이미 살아 있는 객체에 '=' 로 값을 덮어쓰는 것
        - exception-safety 및 객체 수명 관리자 차원에서의 컨테이너의 의의를 준수
    */
    /*
        [ 0 ........ size-1 ]   initialized
        [ size ..... capacity-1 ] uninitialized

        1) [0, size) : initialized
        2) [idx, size) : initialized
        3) [size, capacity) : uninitialized

        - 하나만 삽입하는 함수이므로 construct는 1회만 이루어짐 (reserve가 필요한 경우 제외)
        - [0, idx)는 건들지 않음
        - [idx, size)를 하나씩 뒤로 미룸
        - [size]를 제외하고는 이미 객체가 생성되어 있는 상태이므로 initialize할 필요 x
    */
    // pos > size인 경우는 UB (range-checked 함수가 아님)
    template <class _Tp, class _Alloc>
    void vector<_Tp, _Alloc>::_insert_aux(iterator pos, const _Tp &v)
    {
        _Tp copy_v = v;
        if (_finish == _end_of_storage) // 공간 부족: 새로 할당 + 전체 copy
        {
            // 1. 새로 메모리 공간 할당
            // 2. [0, pos), pos, (pos, finish)에 대해 순서대로 copy 수행
            //    (v가 현재 벡터 공간에 포함된 경우 고려하여 미리 복사본 만들기)
            // 3. 포인터 갱신
            size_t new_cap = _next_capacity(capacity());
            _Tp   *new_start = _allocate(new_cap);
            _Tp   *new_finish = new_start;
            try
            {
                // pos 이전: [start, pos)
                new_finish = ft::uninitialized_copy(_start, pos, new_start);
                // pos
                _construct(new_finish, copy_v);
                ++new_finish;
                // pos 이후: [pos, end)
                new_finish = ft::uninitialized_copy(pos, end(), new_finish);

                // 후처리
                destroy_range(_start, _finish);
                _deallocate(_start, size());
                _start = new_start;
                _finish = new_finish;
                _end_of_storage = _start + new_cap;
            }
            catch (...)
            {
                destroy_range(new_start, new_finish);
                throw;
            }
        }
        else // 공간 충분: copy만 하고 끝 (pos 이후로 한 칸 밀기)
        {
            // 1. 맨 마지막에 객체 생성
            // 2. pos 이후로 copy 수행
            // 3. pos 자리에 value 대입
            _construct(_finish, v);
            ++_finish;
            ft::copy_backward(pos, _finish - 1, _finish);
            *(pos) = copy_v;
        }
    }

} // namespace ft

#endif

#ifndef FT_VECTOR_3PTR_IMPL_H
#define FT_VECTOR_3PTR_IMPL_H

// #include "vector_3ptr.h"
#include "ft_memory.h"

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
    template <class _Tp, class _Alloc>
    void vector<_Tp, _Alloc>::reserve(size_type new_cap)
    {
        if (new_cap > capacity())
        {
            pointer new_start = _allocate_and_copy(new_cap, _start, _finish);
            destroy_range(_start, _finish);
            _deallocate(_start, capacity());
            _start = new_start;
            _finish = _start + size();
            _end_of_storage = _start + new_cap;
        }
    }

    // -------------------- member operator -------------------- //
    // Copy assignment operator
    template <class _Tp, class _Alloc>
    vector<_Tp, _Alloc> &vector<_Tp, _Alloc>::operator=(const vector<_Tp, _Alloc> &x)
    {
        // 공간이 불충분한 경우: 재할당 및 객체 생성
        // 공간은 충분하나 활성화된 원소 개수가 과다한 경우: 원본 객체 복사 및 초과분 destory
        // 공간은 충분하나 활성화된 원소 개수가 적은 경우: 부족분 ft::uninitialized_copy 수행
        if (&x != this)
        {
            if (capacity() < x.size())
            {
                pointer new_start = _allocate_and_copy(x.size(), x.begin(), x.end());
                (void)new_start;
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
        insert(position, _Tp());
    }

    // range insert
    template <class _Tp, class _Alloc>
    template <class _InputIt>
    void
    vector<_Tp, _Alloc>::insert(iterator pos, _InputIt first, _InputIt last,
                                typename ft::enable_if<!ft::is_integral<_InputIt>::value, void>::type *)
    {
        // 한번만 읽을 수 있는 iterator인 경우: 하나씩 insert
        // 여러번 읽을 수 있는 iterator인 경우: ft::distance를 얻고 일괄 처리
        // tag dispatch
        _insert_dispatch(pos, first, last, typename iterator_traits<_InputIt>::iterator_category());
    }

    template <class _Tp, class _Alloc>
    template <class _InputIt>
    void vector<_Tp, _Alloc>::_insert_dispatch(iterator pos, _InputIt first, _InputIt last,
                                               ft::input_iterator_tag)
    {
        // 한번만 읽을 수 있는 iterator인 경우: 하나씩 insert
        while (first != last)
        {
        }
    }

    template <class _Tp, class _Alloc>
    template <class _InputIt>
    void vector<_Tp, _Alloc>::_insert_dispatch(iterator pos, _InputIt first, _InputIt last,
                                               ft::forward_iterator_tag)
    {
        // 여러번 읽을 수 있는 iterator인 경우: ft::distance를 얻고 일괄 처리
        (void)pos;
        (void)first;
        (void)last;
    }

    // range insert
    template <class _Tp, class _Alloc>
    void vector<_Tp, _Alloc>::insert(iterator pos, size_type n, const _Tp &x)
    {
        (void)pos;
        (void)n;
        (void)x;
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
        size_t len = last - first;
        copy(last + 1, end(), first);
        destroy_range(end() - len, end());
        return first;
    }

    template <class _Tp, class _Alloc>
    void vector<_Tp, _Alloc>::resize(size_type new_size, const _Tp &x)
    {
        if (new_size < size())
            // new_size < size: 넘치는 만큼 destroy
            erase(end() - (new_size - size()), end());
        else
            // new_size > size: 부족한 만큼 값 x 삽입
            insert(end(), size() - new_size, x);
    }

    /* ------------------ Internal private methods ------------------  */

    template <class _Tp, class _Alloc>
    void vector<_Tp, _Alloc>::_range_check(size_type n) const
    {
        // todo: 출력 메세지 수정
        if (n >= this->size())
        {
            // std::ostringstream oss;
            // oss << "error msg";
            throw std::out_of_range("");
        }
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

    template <class _Tp, class _Alloc>
    void vector<_Tp, _Alloc>::_insert_aux(iterator pos, const _Tp &v)
    {
        if (_finish == _end_of_storage) // 공간 부족: 새로 할당 + 전체 copy
        {
            // 1. 새로 메모리 공간 할당
            // 2. [0, pos), pos, (pos, finish)에 대해 순서대로 copy 수행
            //    (v가 현재 벡터 공간에 포함된 경우 고려하여 미리 복사본 만들기)
            // 3. 포인터 갱신
            size_t new_cap = _next_capacity(capacity());
            _Tp   *new_start = _allocate(new_cap);
            _Tp   *new_finish = new_start;
            _Tp    copy_v = v;
            try
            {
                // pos 이전
                new_finish = ft::uninitialized_copy(_start, pos, new_start);
                // pos
                _construct(new_finish, copy_v);
                ++new_finish;
                // pos 이후
                new_finish = ft::uninitialized_copy(pos + 1, end(), new_finish);
                // 포인터 갱신
                _start = new_start;
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
        }
    }

} // namespace ft

#endif

#ifndef FT_DESTROY_RANGE_H
#define FT_DESTROY_RANGE_H

// std::destroy를 참조

/*
    - 객체가 이미 정상적으로 존재하고 있는 상황에서 lifetime을 종료할 때 호출
    - exception(예외) 처리 용도 아님!
*/

/*
    - destroy는 되돌릴 수 없는 연산이다.
    - exception-safety를 위해, 가장 마지막 단계에서만 사용
*/

namespace ft
{
    template <class T>
    void destroy_range(T *first, T *last)
    {
        for (; first != last; ++first)
            first->~T();
    }

    // iterator가 아닌 포인터로 처리하는 이유:
    // -> iterator는 객체가 살아있음을 전제로 하는 추상화이기 때문
    template <class _Alloc>
    void destroy_range(_Alloc &alloc, typename _Alloc::value_type *first,
                       typename _Alloc::value_type *last)
    {
        for (; first != last; ++first)
        {
            alloc.destroy(first);
        }
    }
} // namespace ft

#endif

#ifndef FT_UNINITIALIZED_COPY_H
#define FT_UNINITIALIZED_COPY_H

#include "iterator_traits.h"

/*
    - uninitialized = 메모리 확보는 됐고, 객체는 아직 생성 안 됨
    - uninitialized_copy = 그 자리에 주어진 iterator가 가리키는 값들을 copy
*/

/*
    *d_first = *first; (X)
    - 위와 같이 operator=를 호출하려면 좌측에 이미 생성된 객체가 있어야 함
    - 그러나 uninitialized_copy이므로 객체 생성이 되어있지 않다
*/

namespace ft
{
    template <class InputIt, class NoThrowForwardIt>
    NoThrowForwardIt uninitialized_copy(InputIt first, InputIt last, NoThrowForwardIt d_first)
    {
        typedef typename iterator_traits<InputIt>::value_type T;
        NoThrowForwardIt                             cur = d_first;

        try
        {
            while (first != last)
            {
                ::new (cur) T(*first);
                ++cur;
                ++first;
            }
            return cur;
        }
        catch (...)
        {
            while (d_first != cur)
            {
                d_first->~T();
                ++d_first;
            }
            throw;
        }
    }

} // namespace ft

#endif

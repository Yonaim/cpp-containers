#ifndef FT_COPY_BACKWARD_H
#define FT_COPY_BACKWARD_H

// std::copy_backward를 참조

/*
    - [first, last) -> [d_last-n, d_last)
    - copy 순서는 reverse order (last 요소가 가장 먼저 복사됨)
    - input과 destination의 메모리 영역이 겹치는 경우에도 안전, overlap-safe
    - 마지막으로 복사된 원소(last element copied) 반환
*/

namespace ft
{
    template <class BidirIt1, class BidirIt2>
    BidirIt2 copy_backward(BidirIt1 first, BidirIt1 last, BidirIt2 d_last)
    {
        while (first != last)
            *(--d_last) = *(--last);
        return d_last;
    }
} // namespace ft

#endif

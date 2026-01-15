#ifndef FT_COPY_H
#define FT_COPY_H

// std::copy를 참조

/*
    - [first, last) -> [d_last-n, d_last)
    - copy 순서는 정방향 (first 요소가 가장 먼저 복사됨)
    - input과 destination의 메모리 영역이 겹치는 경우에는 안전하지 않음
    - 마지막으로 쓴 원소 다음 칸(one past the last element copied) 반환
*/

namespace ft
{
    template <class InputIt, class OutputIt>
    OutputIt copy(InputIt first, InputIt last, OutputIt d_first)
    {
        while (first != last)
            *(d_first++) = *(first++);
        return d_first;
    }
} // namespace ft

#endif

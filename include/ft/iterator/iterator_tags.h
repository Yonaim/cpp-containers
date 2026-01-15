#ifndef FT_ITERATOR_TAGS_H
#define FT_ITERATOR_TAGS_H

/*
    iterator 카테고리 (아래로 갈수록 권한이 더 많음)
    - input iterator: 한번만 읽기 가능, 단방향 (++it만 됨)
    - forward iterator: 여러번 읽기 가능, 단방향 (++it만 됨)
    - bidirectional iterator: 여러번 읽기 가능, 양방향(++it, --it 둘다 됨)
    - random access iterator: 여러번 읽기 가능, 임의 접근 가능(it + n, it[n], it1 - it2 가능)
*/

namespace ft
{
    struct input_iterator_tag
    {
    };

    struct output_iterator_tag
    {
    };

    struct forward_iterator_tag : input_iterator_tag
    {
    };

    struct bidirectional_iterator_tag : forward_iterator_tag
    {
    };

    struct random_access_iterator_tag : bidirectional_iterator_tag
    {
    };

    struct contiguous_iterator_tag : random_access_iterator_tag
    {
    };
} // namespace ft

#endif

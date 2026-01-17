#ifndef FT_ITERATOR_BASE_H
#define FT_ITERATOR_BASE_H

namespace ft
{
    template <class _Tp, class _Distance>
    struct random_access_iterator
    {
        typedef random_access_iterator_tag iterator_category;
        typedef _Tp                        value_type;
        typedef _Distance                  difference_type;
        typedef _Tp                       *pointer;
        typedef _Tp                       &reference;
    };
} // namespace ft

#endif

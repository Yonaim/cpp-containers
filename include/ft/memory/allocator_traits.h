#ifndef FT_ALLOCATOR_TRAITS_H
#define FT_ALLOCATOR_TRAITS_H

#include "type_traits.h"

// 컨테이너 내부 구현용
namespace ft
{
    template <class T, class Alloc>
    struct _Alloc_traits
    {
        typedef typename Alloc::template rebind<T>::other allocator_type;
        enum
        {
            _instanceless = ft::is_empty<Alloc>::value
        };
    };
} // namespace ft

#endif

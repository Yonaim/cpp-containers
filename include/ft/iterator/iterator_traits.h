#ifndef FT_ITERATOR_TRAITS_H
#define FT_ITERATOR_TRAITS_H

#include <cstddef>
#include <iterator>
#include "remove_cv.h"
#include "iterator_tags.h"

/*
nested type을 그대로 갖고온다.
- difference_type: iterator의 차 (보통 ptrdiff)
- value_type: iterator가 가리키는 타입
- pointer: 요소를 가리키는 포인터 타입
- reference: 요소를 참조하는 레퍼런스 타입
- iterator_category: iterator의 기능 수준 분류
*/

/*
참고: iterator category 계층도

input_iterator_tag
 ├─ forward_iterator_tag
 │   └─ bidirectional_iterator_tag
 │       └─ random_access_iterator_tag
output_iterator_tag
*/

/*
중요!! 표준에서 value_type은 CV 한정자를 제거한 순수한 타입임
(컨테이너 원소 타입 자체를 나타내야하기 때문)
근거: For a pointer type T*, iterator_traits<T*> shall define value_type as T
*/

/*
volatile 객체를 대상으로 하는 iterator는 지원되지 않는다.
메모리 최적화 속성 -> iterator와 같이 값에 반복적으로 접근/수정하는게 의미 없기 때문
*/
namespace ft
{
    // Iter: iterator 혹은 const_iterator 타입만 허용
    // 이외 타입이 오는 경우, 아래 type alias에서 컴파일 오류
    template <class Iter>
    struct iterator_traits
    {
        typedef typename Iter::difference_type   difference_type;
        typedef typename Iter::value_type        value_type;
        typedef typename Iter::pointer           pointer;
        typedef typename Iter::reference         reference;
        typedef typename Iter::iterator_category iterator_category;
    };

    // specialization for mutable pointer
    template <class T>
    struct iterator_traits<T *>
    {
        typedef std::ptrdiff_t                  difference_type;
        typedef typename ft::remove_cv<T>::type value_type; // volatile 한정자 때문에 제거 필요
        typedef T                              *pointer;
        typedef T                              &reference;
        typedef ft::random_access_iterator_tag iterator_category;
    };

    // specialization for const pointer
    template <class T>
    struct iterator_traits<const T *>
    {
        typedef std::ptrdiff_t                  difference_type;
        typedef typename ft::remove_cv<T>::type value_type;
        typedef const T                        *pointer;
        typedef const T                        &reference;
        typedef ft::random_access_iterator_tag iterator_category;
    };

} // namespace ft

#endif

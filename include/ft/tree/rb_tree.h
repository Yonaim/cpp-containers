#ifndef FT_RB_TREE_H
#define FT_RB_TREE_H

#include "allocator.h"
#include "distance.h"
#include "equal.h"
#include "is_empty.h"
#include "iterator_tags.h"
#include "lexicographical_compare.h"
#include "pair.h"
#include "rb_tree_iterator.h"
#include "rb_tree_node.h"
#include "reverse_iterator.h"

/*
    - STL 구현상 tree는 map, set에서만 쓰임
    - RBT(Red Black Tree)만 사용
    - 따라서 stl_map.h = 사실상 RBT 구현 $
    - map, set의 내부 처리
        - 트리 알고리즘 자체는 동일
        - value 처리만 다르다 (compare를 어떤 타입에서 하고, 어떤 부분을 복사하는지 등)
*/

/*
    - Rb_tree_header: 헤더 노드
    - Rb_tree_alloc_base: 노드 메모리 할당 / 해제만 담당(역할 분리)
        - instance 버전 (allocator를 멤버로 가짐)
        - instanceless 버전 (allocator를 멤버로 가지지 않음)
    - Rb_tree_base: intanceless 최적화를 위해 존재, 분기 처리하여 해당하는 alloc_base 상속 받음
    - Rb_tree: 메인 엔진
*/

/*
    EBO(Empty Base Optimization)
    : 빈 클래스(empty class)를 상속받을 때, 그 클래스가 차지하는 메모리를 0바이트로 최적화.

    원래는, 빈 클래스라도 기본적으로 1바이트가 필요함!!!
    객체는 고유한 주소를 가져야하기 때문이다.

    그런데 '상속' 구조에서는 이를 최적화할 수 있다.
    상속 받는 클래스가 어차피 메모리를 차지하고 있기 때문에 고유한 주소를 이미 갖고 있음.
        -> empty class에 대해 메모리를 할당할 필요가 x.
*/

#include <cstddef>

namespace ft
{
    // =============================== Rb_tree_header =================================

    /*
        [헤더 노드]
        - 진짜 노드가 아닌 트리의 메타 정보를 저장하는 노드
        - 멤버의 이름(parent, left, right)과 저장 데이터는 관계없음
        - header->parent->parent = parent라는 특이한 값을 가짐
        - '이 weird한 구조 + color가 RED'를 header를 판별하는데 사용
        - 각 멤버
            - header->_M_parent = root
            - header->_M_left   = leftmost (begin)
            - header->_M_right  = rightmost (end - 1)
            - header->_M_color  = RED     // 오직 header만 RED면서 weird한 구조
            - root->_M_parent   = header  // header 판별 조건
    */
    /*
         - 추후 노드 추가시, leftmost(begin) 값이 바뀜
         - end는 늘 언제나 header와 동일
    */
    struct _Rb_tree_header
    {
        typedef _Rb_tree_node_base *_Base_ptr;
        _Rb_tree_node_base          _base_node;
        size_t                      count;

        _Rb_tree_header() : _base_node(), count(0) {}
    };

    // =============================== Rb_tree_alloc_base =================================

    template <class _Tp, class _Alloc, bool _instanceless>
    class _Rb_tree_alloc_base;

    template <class _Tp, class _Alloc>
    class _Rb_tree_alloc_base<_Tp, _Alloc, false>
    {
      public:
        typedef _Alloc allocator_type;
        typedef _Tp    value_type;

        typedef
            typename _Alloc::template rebind<_Rb_tree_node<value_type> >::other node_allocator_type;
        typedef _Rb_tree_node<value_type>                                      node_type;

        allocator_type get_allocator() const { return allocator_type(_node_allocator); }

        _Rb_tree_alloc_base(const allocator_type &a) : _node_allocator(a) {}

      protected:
        node_allocator_type _node_allocator;

        node_type *_get_node() { return _node_allocator.allocate(1); }
        void       _put_node(node_type *p) { return _node_allocator.deallocate(p, 1); }
    };

    template <class _Tp, class _Alloc>
    class _Rb_tree_alloc_base<_Tp, _Alloc, true>
    {
      public:
        typedef _Alloc allocator_type;
        typedef _Tp    value_type;

        typedef
            typename _Alloc::template rebind<_Rb_tree_node<value_type> >::other node_allocator_type;
        typedef _Rb_tree_node<value_type>                                      node_type;

        allocator_type get_allocator() const { return allocator_type(); }

        _Rb_tree_alloc_base(const allocator_type &a) { (void)a; }

      protected:
        node_type *_get_node()
        {
            node_allocator_type a;
            return a.allocate(1);
        }
        void _put_node(node_type *p)
        {
            node_allocator_type a;
            return a.deallocate(p, 1);
        }
    };

    // =============================== Rb_tree_base =================================

    template <class _Tp, class _Alloc>
    class _Rb_tree_base : private _Rb_tree_alloc_base<_Tp, _Alloc, ft::is_empty<_Alloc>::value>
    {
        typedef _Rb_tree_alloc_base<_Tp, _Alloc, ft::is_empty<_Alloc>::value> Base;

      protected:
        typedef typename Base::node_type node_type;

        _Rb_tree_header _header;

        using Base::_get_node;
        using Base::_put_node;
        using Base::get_allocator;

      public:
        _Rb_tree_base(const _Alloc &alloc = _Alloc()) : Base(alloc), _header() {}
        ~_Rb_tree_base() {}
    };

    // =============================== Rb_tree =================================

    // Compare(x, y): x < y의 결과를 bool로 반환
    template <typename _Key, typename _Value, typename _KeyOfValue, typename _Compare,
              typename _Alloc = ft::allocator<_Value> >
    class _Rb_tree : protected _Rb_tree_base<_Value, _Alloc>
    {
        typedef _Rb_tree_base<_Value, _Alloc> _Base;
        using _Base::_header;

      public:
        // key & value
        typedef _Key              key_type;
        typedef _Value            value_type;
        typedef value_type       *pointer;
        typedef const value_type *const_pointer;
        typedef value_type       &reference;
        typedef const value_type &const_reference;

        typedef _Alloc                           allocator_type;
        typedef typename _Alloc::difference_type difference_type;
        typedef typename _Alloc::size_type       size_type;
        typedef _Rb_tree_node_base              *_Base_ptr;
        typedef ft::_Rb_tree_node<_Value>        _Rb_tree_node;
        typedef _Rb_tree_node                   *_Node_ptr;
        typedef _Rb_tree_color                   _Color_type;

        typedef _Rb_tree_iterator<_Value, _Value &, _Value *>             iterator;
        typedef _Rb_tree_iterator<_Value, const _Value &, const _Value *> const_iterator;
        typedef ft::reverse_iterator<iterator>                            reverse_iterator;
        typedef ft::reverse_iterator<const_iterator>                      const_reverse_iterator;

        // ================================ API ================================
        _Rb_tree();
        _Rb_tree(const _Rb_tree &x);
        explicit _Rb_tree(const _Compare &comp, const _Alloc &alloc = _Alloc());
        _Rb_tree &operator=(const _Rb_tree &x);
        ~_Rb_tree() { clear(); };

        void swap(_Rb_tree &other);
        void clear();

        // 둘다 key값을 비교하는 객체
        // value_comp는 value를 받아 거기서 key를 꺼내 비교
        _Compare key_comp() const { return _key_compare; }
        _Compare value_comp() const { return _key_compare; }

        iterator               begin();
        const_iterator         begin() const;
        iterator               end();
        const_iterator         end() const;
        reverse_iterator       rbegin();
        const_reverse_iterator rbegin() const;
        reverse_iterator       rend();
        const_reverse_iterator rend() const;
        bool                   empty() const;
        size_type              size() const;

        iterator       find(const key_type &k);
        const_iterator find(const key_type &k) const;

        iterator                             lower_bound(const key_type &k);
        const_iterator                       lower_bound(const key_type &k) const;
        iterator                             upper_bound(const key_type &k);
        const_iterator                       upper_bound(const key_type &k) const;
        pair<iterator, iterator>             equal_range(const key_type &k);
        pair<const_iterator, const_iterator> equal_range(const key_type &k) const;

        pair<iterator, bool> insert_unique(const value_type &v);
        iterator             insert_equal(const value_type &v);
        iterator             insert_unique(iterator position, const value_type &v);
        iterator             insert_equal(iterator position, const value_type &v);

        template <class _InputIterator>
        void insert_unique(_InputIterator first, _InputIterator last);
        template <class _InputIterator>
        void insert_equal(_InputIterator first, _InputIterator last);

        void      erase(iterator position);
        size_type erase(const key_type &k);
        void      erase(iterator first, iterator last);
        void      erase(const key_type *first, const key_type *last);

      protected:
        _Compare _key_compare;

        _Base_ptr &_root() const;
        _Node_ptr &_root_node() const;
        _Base_ptr &_leftmost() const;
        _Base_ptr &_rightmost() const;

        // Node pointer helper
        static _Node_ptr      &_left(_Node_ptr n);
        static _Node_ptr      &_right(_Node_ptr n);
        static _Node_ptr      &_parent(_Node_ptr n);
        static reference       _value(_Node_ptr n);
        static const key_type &_key(_Node_ptr n);
        static _Color_type    &_color(_Node_ptr n);

        // Base pointer helper
        static _Node_ptr      &_left(_Base_ptr b);
        static _Node_ptr      &_right(_Base_ptr b);
        static _Node_ptr      &_parent(_Base_ptr b);
        static reference       _value(_Base_ptr b);
        static const key_type &_key(_Base_ptr b);
        static _Color_type    &_color(_Base_ptr b);

        static const key_type &_key(const value_type &v);
        static _Node_ptr       _minimum(_Node_ptr x);
        static _Node_ptr       _maximum(_Node_ptr x);

      private:
        // sentinel = header
        _Base_ptr _sentinel() { return &_header._base_node; }
        _Base_ptr _sentinel() const { return const_cast<_Base_ptr>(&_header._base_node); }
        iterator  _insert(_Base_ptr x_hint, _Base_ptr y, const value_type &v);
        void      _empty_initialize();
        _Node_ptr _create_node(const value_type &v);
        _Node_ptr _clone_node(_Node_ptr orig);
        void      _destroy_node(_Node_ptr n_ptr);
        _Node_ptr _copy(_Node_ptr x, _Node_ptr p);

        // rotate
        void _rotate_left(_Base_ptr x);
        void _rotate_right(_Base_ptr x);

        // fixup
        void      _rebalance_for_insert(_Base_ptr x, _Base_ptr &root);
        _Base_ptr _rebalance_for_erase(_Base_ptr z);
        void      _erase_subtree(_Base_ptr x);
    };

    template <class _Key, class _Value, class _KeyOfValue, class _Compare, class _Alloc>
    inline bool operator==(const _Rb_tree<_Key, _Value, _KeyOfValue, _Compare, _Alloc> &x,
                           const _Rb_tree<_Key, _Value, _KeyOfValue, _Compare, _Alloc> &y)
    {
        return (x.size() == y.size() && ft::equal(x.begin(), x.end(), y.begin()));
    }

    template <class _Key, class _Value, class _KeyOfValue, class _Compare, class _Alloc>
    inline bool operator<(const _Rb_tree<_Key, _Value, _KeyOfValue, _Compare, _Alloc> &x,
                          const _Rb_tree<_Key, _Value, _KeyOfValue, _Compare, _Alloc> &y)
    {
        return ft::lexicographical_compare(x.begin(), x.end(), y.begin(), y.end());
    }

    template <class _Key, class _Value, class _KeyOfValue, class _Compare, class _Alloc>
    inline bool operator!=(const _Rb_tree<_Key, _Value, _KeyOfValue, _Compare, _Alloc> &x,
                           const _Rb_tree<_Key, _Value, _KeyOfValue, _Compare, _Alloc> &y)
    {
        return !(x == y);
    }

    template <class _Key, class _Value, class _KeyOfValue, class _Compare, class _Alloc>
    inline bool operator>(const _Rb_tree<_Key, _Value, _KeyOfValue, _Compare, _Alloc> &x,
                          const _Rb_tree<_Key, _Value, _KeyOfValue, _Compare, _Alloc> &y)
    {
        return (y < x);
    }

    template <class _Key, class _Value, class _KeyOfValue, class _Compare, class _Alloc>
    inline bool operator<=(const _Rb_tree<_Key, _Value, _KeyOfValue, _Compare, _Alloc> &x,
                           const _Rb_tree<_Key, _Value, _KeyOfValue, _Compare, _Alloc> &y)
    {
        return !(x > y);
    }

    template <class _Key, class _Value, class _KeyOfValue, class _Compare, class _Alloc>
    inline bool operator>=(const _Rb_tree<_Key, _Value, _KeyOfValue, _Compare, _Alloc> &x,
                           const _Rb_tree<_Key, _Value, _KeyOfValue, _Compare, _Alloc> &y)
    {
        return !(x < y);
    }

} // namespace ft

#include "rb_tree_impl.h"
#endif

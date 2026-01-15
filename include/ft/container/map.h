// https://github.com/gcc-mirror/gcc/blob/releases/gcc-3.0/libstdc%2B%2B-v3/include/bits/stl_map.h
#ifndef FT_MAP_H
#define FT_MAP_H

#include <cstddef>
#include <functional>
#include <stdexcept>
#include "functional.h"
#include "rb_tree.h"
#include "type_traits.h"


// todo: exception-safe 처리

/*
- A sorted associative container that contains key-value pairs with 'unique' keys.
    - associative: key를 기준으로 데이터를 저장하고 찾음
- Keys are sorted by using the comparison function 'Compare'
- Search, removal, and insertion operations have logarithmic complexity
    - logarithmic complexity: 로그 복잡도, O(logN)
- Maps are usually implemented as Red–black trees.
*/

namespace ft
{
    // map은 tree의 thin wrapper
    template <class Key, class T, class Compare = std::less<Key>,
              class Allocator = ft::allocator<ft::pair<const Key, T> > >
    class map
    {
      public:
        // typedefs:
        typedef Key                    key_type;
        typedef T                      mapped_type;
        typedef ft::pair<const Key, T> value_type;
        typedef Compare                key_compare;
        typedef Allocator              allocator_type;

        // value의 key(first)끼리 비교하는 함수 객체 타입
        // binary_function: 인자 2개인 함수 객체 (functor) 어댑터
        // nested class
        class value_compare : std::binary_function<value_type, value_type, bool>
        {
            friend class map; // 생성자에 접근 필요
          public:
            /*
            C++98 STL functor 대부분은 binary_function이 제공하는 typedef를 그대로 사용한다.
            따라서 functor 내부에 typedef를 직접 작성할 필요는 없다.

            하지만 map::value_compare처럼 표준 문서에서 typedef가 명시된
            'public API용 functor'의 경우에는 typedef를 functor 내부에 직접 선언해야 한다.
            */
            typedef bool       result_type;
            typedef value_type first_argument_type;
            typedef value_type second_argument_type;

          protected:
            Compare comp;
            // Initializes the internal instance of the comparator to c.
            value_compare(Compare c) : comp(c) {};

          public:
            // Compares lhs.first and rhs.first by calling the stored comparator.
            bool operator()(const value_type &lhs, const value_type &rhs) const
            {
                return comp(lhs.first, rhs.first);
            }
        };

      private:
        // Representative type: 실제 구현을 대표(represent)하는 타입
        typedef _Rb_tree<key_type, value_type, Select1st<value_type>, Compare> _Rep_type;
        _Rep_type                                                              _tree;

      public:
        // _Rep_type의 내부의 typedef을 그대로 가져와 재사용
        typedef typename _Rep_type::pointer                pointer;
        typedef typename _Rep_type::const_pointer          const_pointer;
        typedef typename _Rep_type::reference              reference;
        typedef typename _Rep_type::const_reference        const_reference;
        typedef typename _Rep_type::iterator               iterator;
        typedef typename _Rep_type::const_iterator         const_iterator;
        typedef typename _Rep_type::reverse_iterator       reverse_iterator;
        typedef typename _Rep_type::const_reverse_iterator const_reverse_iterator;
        typedef typename _Rep_type::size_type              size_type;
        typedef typename _Rep_type::difference_type        difference_type;

        // =========================== allocation/deallocation ============================

        // Constructs an empty container
        map() : _tree(Compare(), Allocator()) {};
        explicit map(const Compare &comp, const Allocator &alloc = Allocator())
            : _tree(comp, alloc) {};

        // Constructs the container with the contents of the range [first, last)
        template <class InputIt>
        map(InputIt first, InputIt last, const Compare &comp, const Allocator &alloc = Allocator())
        {
            _tree.insert_unique(first, last);
        };

        // 문서에는 없으나 편의상 구현
        template <class InputIt>
        map(InputIt first, InputIt last) : _tree(Compare(), Allocator())
        {
            _tree.insert_unique(first, last);
        };

        // Constructs the container with the copy of the contents of other
        map(const map &other) : _tree(other._tree) {}

        map &operator=(const map &other)
        {
            _tree = other._tree;
            return *this;
        }

        // ========================== Accessors ===========================

        allocator_type get_allocator() const { return _tree.get_allocator(); }

        // observers
        key_compare   key_comp() const { return _tree.key_comp(); }
        value_compare value_comp() const { return _tree.value_comp(); }

        // element access
        // Returns a reference to the mapped value of the element with specified key. If no such
        // element exists, an exception of type std::out_of_range is thrown.
        T &at(const Key &key)
        {
            iterator it = _tree.find(key);
            if (it == _tree.end())
                throw std::out_of_range("map::at");
            return it->second;
        }

        const T &at(const Key &key) const
        {
            const_iterator it = _tree.find(key);
            if (it == _tree.end())
                throw std::out_of_range("map::at");
            return it->second;
        }

        /*
        - operator[] must run in logarithmic time
        - 탐색에 O(logN)의 시간이 걸림
        - 삽입을 위해 재탐색을 한다면 비효율적이므로, 힌트 기반 삽입을 사용
        - 힌트 기반 삽입의 가능성이 있으므로 find가 아닌 lower_bound가 적절
        */
        T &operator[](const Key &key)
        {
            iterator it = _tree.lower_bound(key);
            /*
                lower_bound: '처음으로' 해당 키보다 큰 키가 나오면 반환
                - end()가 반환될 시, 맨 끝을 의미 (해당 key와 같은 것은 없다)
                - 반환된 위치의 key가 주어진 key와 일치하는지 추가확인을 통해 존재여부 파악 가능
            */
            // key not found -> insert
            // key_comp()는 functor 객체 (오버로딩된 operator()를 호출)
            if (it == _tree.end() || key_comp()(it->first, key))
                it = _tree.insert_unique(it, value_type(key, mapped_type()));
            return it->second;
        }

        // ========================== Iterators ===========================

        // cbegin, cend는 C++11
        iterator               begin() { return _tree.begin(); }
        const_iterator         begin() const { return _tree.begin(); }
        reverse_iterator       rbegin() throw() { return _tree.rbegin(); }
        const_reverse_iterator rbegin() const throw() { return _tree.rbegin(); }
        iterator               end() { return _tree.end(); }
        const_iterator         end() const { return _tree.end(); }
        reverse_iterator       rend() throw() { return _tree.rend(); }
        const_reverse_iterator rend() const throw() { return _tree.rend(); }

        // ========================== Capacity ===========================
        bool      empty() const { return _tree.empty(); }
        size_type size() const { return _tree.size(); }
        size_type max_size() const { return _tree.max_size(); }

        // ========================== Modifiers ===========================

        void clear() { _tree.clear(); };

        // Inserts value
        ft::pair<iterator, bool> insert(const value_type &value)
        {
            return _tree.insert_unique(value);
        }

        // Inserts value in the position as close as possible to the position just prior to pos.
        iterator insert(iterator pos, const value_type &value)
        {
            return _tree.insert_unique(pos, value);
        }

        // Inserts elements from range [first, last)
        template <class InputIt>
        void insert(InputIt first, InputIt last)
        {
            return _tree.insert_unique(first, last);
        }

        // Removes the element at pos.
        void erase(iterator pos) { _tree.erase(pos); }

        // Removes the elements in the range [first, last)
        void erase(iterator first, iterator last) { _tree.erase(first, last); }

        // Removes the element (if one exists) with the key equivalent to key
        size_type erase(const Key &key) { return _tree.erase(key); }

        // Exchanges the contents of the container with those of other
        void swap(map &other) { _tree.swap(other._tree); }

        // ========================== Lookup ===========================

        // 중복 키가 존재하지 않으므로 반환 값은 0 혹은 1
        size_type count(const Key &key) const { return (_tree.find(key) == _tree.end() ? 0 : 1); }

        iterator       find(const Key &key) { return _tree.find(key); }
        const_iterator find(const Key &key) const { return _tree.find(key); }

        ft::pair<iterator, iterator> equal_range(const Key &key) { return _tree.equal_range(key); }
        ft::pair<const_iterator, const_iterator> equal_range(const Key &key) const
        {
            return _tree.equal_range(key);
        }
        iterator       lower_bound(const Key &key) { return _tree.lower_bound(key); }
        const_iterator lower_bound(const Key &key) const { return _tree.lower_bound(key); }
        iterator       upper_bound(const Key &key) { return _tree.upper_bound(key); }
        const_iterator upper_bound(const Key &key) const { return _tree.upper_bound(key); }

        // friend declaration for non-member function
        /*
            friend 선언을 사용하는 이유:
            - private 멤버인 _tree에 접근할 수 있도록 하기 위해서
            - getter를 pulic API로 제공하기엔, 캡슐화가 심하게 깨짐
        */
        // non-member function이므로 반드시 template으로 제공
        template <class K1, class T1, class C1, class A1>
        friend bool operator==(const map<K1, T1, C1, A1> &lhs, const map<K1, T1, C1, A1> &rhs);

        template <class K1, class T1, class C1, class A1>
        friend bool operator>(const map<K1, T1, C1, A1> &lhs, const map<K1, T1, C1, A1> &rhs);
    };

    template <class K1, class T1, class C1, class A1>
    bool operator==(const map<K1, T1, C1, A1> &lhs, const map<K1, T1, C1, A1> &rhs)
    {
        return lhs._tree == rhs._tree;
    }

    template <class K1, class T1, class C1, class A1>
    bool operator>(const map<K1, T1, C1, A1> &lhs, const map<K1, T1, C1, A1> &rhs)
    {
        return lhs._tree > rhs._tree;
    }

    template <class K1, class T1, class C1, class A1>
    bool operator!=(const map<K1, T1, C1, A1> &lhs, const map<K1, T1, C1, A1> &rhs)
    {
        return !(lhs == rhs);
    }

    template <class K1, class T1, class C1, class A1>
    bool operator<(const map<K1, T1, C1, A1> &lhs, const map<K1, T1, C1, A1> &rhs)
    {
        return rhs > lhs;
    }

    template <class K1, class T1, class C1, class A1>
    bool operator<=(const map<K1, T1, C1, A1> &lhs, const map<K1, T1, C1, A1> &rhs)
    {
        return !(lhs > rhs);
    }

    template <class K1, class T1, class C1, class A1>
    bool operator>=(const map<K1, T1, C1, A1> &lhs, const map<K1, T1, C1, A1> &rhs)
    {
        return !(lhs < rhs);
    }
} // namespace ft

#endif

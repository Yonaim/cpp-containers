#ifndef FT_VECTOR_SIZED_H
#define FT_VECTOR_SIZED_H

#include <sstream>
#include <stdexcept>
#include "algorithm.h"
#include "ft_memory.h"
#include "iterator.h"
#include "type_traits.h"


/*
    legacy version: size, capacity를 저장
*/

/*
    function types:
    - constructor
    - destructor
    - element access
    - iterators
    - capacity
    - modifiers
    - operators (non-member)
*/

// legacy utils
namespace ft
{
    template <class _Alloc>
    static typename _Alloc::value_type *uninitialized_fill(typename _Alloc::value_type *dest,
                                                           typename _Alloc::value_type  value,
                                                           size_t n, _Alloc &alloc)
    {
        typename _Alloc::value_type *cur = dest;
        construct_guard<_Alloc>      guard(alloc, dest);

        for (size_t i = 0; i < n; ++i)
        {
            alloc.construct(cur + i, value);
            guard.bump();
        }
        // 만약 construct 중 실패시 c++ 예외 규칙에 의해 그 즉시 guard의 소멸자가 호출됨 (RAII)
        guard.release();
        return cur;
    }

    // value가 아닌 Allocator 타입이 템플릿 인자인 이유?
    // -> 후자에서 전자를 유도 가능, 전자에서 후자 유도는 불가능 (policy 객체이기 때문에)
    template <class _InputIt, class _Alloc>
    static typename _Alloc::value_type *uninitialized_copy(_InputIt first, _InputIt last,
                                                           typename _Alloc::value_type *dest,
                                                           _Alloc                      &alloc)
    {
        typename _Alloc::value_type *cur = dest;
        construct_guard<_Alloc>      guard(alloc, dest);

        for (; first != last; ++first, ++cur)
        {
            alloc.construct(cur, *first);
            guard.bump();
        }
        // 만약 construct 중 실패시 c++ 예외 규칙에 의해 그 즉시 guard의 소멸자가 호출됨 (RAII)
        guard.release();
        return cur;
    }
} // namespace ft

namespace ft
{
    /*
        - contiguous memory 사용 (bool partial specialization 제외)
        - 원소의 순서를 항상 보장 (order guarantee)
        - 중간 삽입/삭제 시 삭제 위치 뒤쪽의 모든 원소 재배치 -> O(n)
        - 맨 뒤 삽입/삭제 시 평균적으로 O(1)
        - 메모리 공간 부족시 전체 새로 할당 후 다시 대입
        - iterator 뿐만 아니라 offset을 통한 접근도 가능하다
    */

    // T: The type of the elements (must be CopyAssignable & CopyConstructible)
    // Allocator: customizable (default one is std::allocator)
    template <class T, class Allocator = ft::allocator<T>>
    class vector
    {
      public:
        // -------------------- Member types -------------------- //
        typedef T         value_type;
        typedef Allocator allocator_type;

        typedef typename allocator_type::reference       reference;
        typedef typename allocator_type::const_reference const_reference;

        typedef typename allocator_type::pointer       pointer;
        typedef typename allocator_type::const_pointer const_pointer;

        // pointer를 iterator로 사용
        // 즉, random access iterator
        typedef pointer       iterator;
        typedef const_pointer const_iterator;

        typedef ft::reverse_iterator<iterator>       reverse_iterator;
        typedef ft::reverse_iterator<const_iterator> const_reverse_iterator;

        typedef std::size_t    size_type;
        typedef std::ptrdiff_t difference_type;

        // -------------------- Constructors -------------------- //

        // Default constructor
        explicit vector(const Allocator &alloc = Allocator())
            : _allocator(alloc), _data(NULL), _size(0), _capacity(0)
        {
        }

        // Fill constructor (default-inserted elements)
        explicit vector(size_type count, const Allocator &alloc = Allocator())
            : _allocator(alloc), _data(_allocator.allocate(count)), _size(0), _capacity(count)
        {
            try
            {
                uninitialized_fill(_data, value_type(), count, _allocator);
                _size = count;
            }
            catch (...)
            {
                _allocator.deallocate(_data, count);
            }
        }

        // Fill constructor (with specified value)
        /*
            C++98 std::vector에는 vector(count, allocator) 생성자가 없어서
            allocator를 지정하면서 개수와 함께 초기화하려면
            vector(count, value, alloc)을 사용해야 했다.
            이때 기본 초기화를 위해 value로 T()를 명시적으로 전달했다.
           예) vector(42, MyType(), MyAlloc<MyType>());
        */
        vector(size_type count, const T &value, const Allocator &alloc = Allocator())
            : _allocator(alloc), _data(_allocator.allocate(count)), _size(0), _capacity(count)
        {
            try
            {
                // 내부적으로 const allocator 타입을 받으면 안되므로 멤버 _allocator를 넘긴다
                uninitialized_fill(_data, value, count, _allocator);
                _size = count;
            }
            catch (...)
            {
                _allocator.deallocate(_data, count);
            }
        }

        // Range constructor
        /*
            - Fill constructor(vector(size_type n, value_type val))와의 충돌을 방지하기 위해
                enable_if와 is_integral을 이용하여 InputIt 타입이 정수가 아님을 강제
            - 그런데 이 방법은 정수 외에 다른 타입은 못 걸러낸다는 단점이 있음
        */
        /*
            그럼 iterator_traits을 이용하면 되지 않나???
            => iterator_category와 같은 것을 typedef로 만들려고 시도할 때,
               '인스턴스화하는 과정에서' 에러.
                -> SFINAE로 조용히 넘어가는게 아니라 하드 에러가 터진다.
                (하드 에러가 터지는 즉시 컴파일이 중단되므로 우아한 해결책이 아님.)
                (enable_if는 인스턴스는 생성되되 멤버만 없을 뿐으로, SFINAE로 취급)

            그래서 타협안:
                정수만 배제해서 (n, value)와 충돌만 막고 진짜 iterator 여부는 런타임 에러가 나게 둠
        */
        template <class InputIt>
        vector(InputIt first, InputIt last, const Allocator &alloc = Allocator(),
               typename ft::enable_if<!ft::is_integral<InputIt>::value, void>::type * = 0)
            : _allocator(alloc), _data(NULL), _size(0), _capacity(0)
        {
            size_type n = 0;
            for (InputIt it = first; it != last; ++it)
                ++n;
            _capacity = n;
            _data = _allocator.allocate(n);
            try
            {
                uninitialized_copy(first, last, _data, _allocator);
                _size = n;
            }
            catch (...)
            {
                _allocator.deallocate(_data, _capacity);
            }
        }

        // Copy constructor
        vector(const vector &other)
            : _allocator(other._allocator), _size(0), _capacity(other._capacity)
        {
            _data = _allocator.allocate(_capacity);
            try
            {
                uninitialized_copy(other._data, other._data + other._size, _data, _allocator);
                _size = other._size;
            }
            catch (...)
            {
                _allocator.deallocate(_data, _capacity);
            }
        }

        // Copy assignment operator
        vector &operator=(const vector &other)
        {
            if (this == &other)
                return *this;

            if (_capacity < other._size)
            {
                clear();
                _allocator.deallocate(_data, _capacity);
                _data = _allocator.allocate(other._size);
                _capacity = other._size;
            }
            else
                clear();

            try
            {
                uninitialized_copy(other._data, other._data + other._size, _data, _allocator);
            }
            catch (...)
            {
                _allocator.deallocate(_data, _capacity);
            }
            _size = other._size;
            return *this;
        }

        // Destructor
        ~vector()
        {
            destroy_range(_allocator, _data, _data + _size);
            if (_data)
                _allocator.deallocate(_data, _capacity);
        }

        // -------------------- Element access -------------------- //
        reference       operator[](size_type pos) { return _data[pos]; }
        const_reference operator[](size_type pos) const { return _data[pos]; }

        reference at(size_type pos)
        {
            // 에러 메세지는 컴파일러 별로 다르다
            if (pos >= _size)
            {
                std::ostringstream oss;
                oss << "vector::_M_range_check: __n (which is " << pos
                    << ") >= this->size() (which is " << _size << ")";
                throw std::out_of_range(oss.str());
            }
            return _data[pos];
        }

        const_reference at(size_type pos) const
        {
            if (pos >= _size)
                throw std::out_of_range("ft::vector::at: index out of range");
            return _data[pos];
        }

        reference       front() { return _data[0]; }
        const_reference front() const { return _data[0]; }
        reference       back() { return _data[_size - 1]; }
        const_reference back() const { return _data[_size - 1]; }

        T       *data() { return _data; }
        const T *data() const { return _data; }

        // -------------------- Capacity -------------------- //
        size_type size() const { return _size; }
        size_type max_size() const { return _allocator.max_size(); }
        size_type capacity() const { return _capacity; }
        bool      empty() const { return (_size == 0); }

        // Increase the capacity of the vector
        // If new_cap is greater than the current capacity(), new storage is allocated,
        // otherwise the function does nothing.
        /*
            - trivially copyable인 경우: memcpy()를 사용해도 됨.
            - trivially copyable이 아니면: 객체의 lifetime을 잃는 등, 논리적인 복제가 불가
            - 모던 c++에 is_trivially_copyable 함수 존재
            - POD(Plain Old Data)는 trivially copyable
        */
        void reserve(size_type new_cap)
        {
            // 에러 메세지는 컴파일러 별로 다르다
            if (new_cap > max_size())
                throw std::length_error("ft::vector::reserve: length_error");
            if (new_cap <= _capacity)
                return;

            T *new_data = _allocator.allocate(new_cap);
            // POD가 아닌 한, memcpy()와 같은 단순한 메모리 복사는 불가
            // If an exception is thrown, this function has no effect (strong exception guarantee).
            try
            {
                uninitialized_copy(_data, _data + _size, new_data, _allocator);
            }
            catch (...)
            {
                _allocator.deallocate(new_data, new_cap);
                throw; // rethrow (현재 잡은 예외 그대로 던짐)
            }
            // 소멸자는 무조건 성공한다고 가정
            destroy_range(_allocator, _data, _data + _size);
            _allocator.deallocate(_data, _capacity);
            _data = new_data;
            _capacity = new_cap;
        }

        // -------------------- Modifiers -------------------- //

        // 모든 원소 destroy 후 _size = 0으로 설정 (메모리 공간은 해제하지 않는다)
        void clear()
        {
            for (size_type i = 0; i < _size; ++i)
                _allocator.destroy(_data + i);
            _size = 0;
        }

        // adds an element to the end (복사본 저장)
        // growth policy: 1.5배 혹은 2배
        void push_back(const T &value)
        {
            if (_size == _capacity)
            {
                size_type new_cap;
                if (_capacity == 0)
                    new_cap = 1;
                else
                    new_cap = _capacity * 2;
                reserve(new_cap);
            }
            // 여러 개를 처리하는게 아니므로 예외가 발생해도 복구할게 없음, 굳이 guard 쓸 필요 x
            _allocator.construct(_data + _size, value); // (O)
            ++_size;

            // C++98에서는 인자 평가 순서가 보장되지 않음
            // _allocator.construct(_data + (_size++), value); (X)
        }

        // removes the last element
        void pop_back()
        {
            if (_size == 0)
                return;
            // non-trivial 타입일 수도 있으므로 destroy를 한다.
            _allocator.destroy(_data + (_size - 1));
            --_size;
        }

        /*
            - 특정 위치에 단일 삽입
            - 지정한 pos 뒤의 원소들은 한 칸씩 뒤로 밀어야 함
            - 뒤에서부터 복사하여 덮어씌워지는 일을 방지 (새 공간을 할당하지 않고 진행)
            - strong exception safety guarantee 만족
        */
        /*
            - 미리 destroy하고 reverse 이터레이터를 인자로 uninitialized_copy() 호출하는 방식은
                 안되는 이유???
                 -> exception-safety 만족을 위해서는 미리 destroy를 하면 안된다.
                    (destroy는 되돌릴 수 없는 연산!)
            - 가능하면 assignment(대입)을 한다.
                - copy constructor와 copy assignment는 전혀 다름
                - assignment시, 객체를 파괴/새로 생성하지 않고 현재 객체의 값만 교체해주면 됨
                - 이미 살아 있는 객체에 '=' 로 값을 덮어쓰는 것
                - exception-safety 및 객체 수명 관리자 차원에서의 컨테이너의 의의를 준수

            => uninitialized 영역과 initialized 영역을 구분하여 처리한다.
                - uninitialized -> uninitialized_copy
                - initialized -> assignment
                - destroy -> 마지막
        */
        /*
            [ 0 ........ size-1 ]   initialized
            [ size ..... capacity-1 ] uninitialized

            1) [0, size) : initialized
            2) [idx, size) : initialized
            3) [size, capacity) : uninitialized

            - 하나만 삽입하는 함수이므로 construct는 1회만 이루어짐 (reserve가 필요한 경우 제외)
            - [0, idx)는 건들지 않음
            - [idx, size)를 하나씩 뒤로 미룸
                - [size]를 제외하고는 이미 객체가 생성되어 있는 상태이므로 initialize할 필요 x
        */
        // pos > size인 경우는 UB (range-checked 함수가 아님)
        // insert(end(), value)는 push_back과 동일
        iterator insert(const_iterator pos, const T &value)
        {
            const size_type idx = pos - begin();

            if (_size == _capacity)
                reserve(_capacity == 0 ? 1 : _capacity * 2);

            // end()에 삽입이면 그냥 push_back과 동일
            if (idx == _size)
            {
                _allocator.construct(_data + _size, value);
                ++_size;
                return begin() + idx;
            }

            // 1) 맨 뒤 uninitialized 슬롯을 "마지막 원소 복제"로 initialize
            _allocator.construct(_data + _size, _data[_size - 1]);

            // 2) initialized 구간은 assignment로 shift (뒤에서 앞으로)
            // _size == 0이면 항상 idx == _size이므로, 오버플로우 발생 X
            for (size_type i = _size - 1; i > idx; --i)
                _data[i] = _data[i - 1];

            // 3) idx 위치에 value 대입
            _data[idx] = value;
            ++_size;

            // 새로 할당될시 pos는 옛날 메모리를 가리키는 것이므로 다를 수 있음!!
            // 그래서 begin + idx를 리턴한다.
            return begin() + idx;
        }

        // 다수 삽입

        /*
            1) [0, size) : initialized
            2-1) [idx, idx + count) : initialized
            2-2) [idx, size), [size, idx + count) : initialized + uninitialized
            2-3) [idx, idx + count) : uninitialized (idx == size)
            3) [size, capacity) : uninitialized
        */
        iterator insert(const_iterator pos, size_type count, const T &value)
        {
            const size_type idx = pos - begin();

            if (count == 0)
                return const_cast<iterator>(pos);
            if (_size + count > _capacity)
                reserve(_grow_capacity_to(_size + count));

            if (idx == _size) // 1. idx == size
            {
                uninitialized_fill(&_data[idx], value, count, _allocator);
            }
            else if (idx + count < _size) // 2. idx + count < _size
            {
                // 1) 맨 뒤쪽 count개를 uninitialized 영역에 생성
                uninitialized_copy(&_data[_size - count], &_data[_size], &_data[_size], _allocator);

                try
                {
                    // 2) [idx, size - count)를 [idx + count, size)로 shift
                    for (size_t i = _size - count; i > idx; --i)
                        _data[i + count - 1] = _data[i - 1];

                    // 3) [idx, idx + count)를 value로 채우기
                    for (size_type i = idx; i < idx + count; ++i)
                        _data[i] = value;
                }
                catch (...)
                {
                    destroy_range(_allocator, _data + _size, _data + _size + count);
                    throw;
                }
            }
            else // 3. idx < _size && idx + count >= _size
            {
                // [idx, size) : initialized
                // [size, idx + count) : uninitialized
                // [idx + count, size + count) : uninitialized

                const size_t extra_cnt = idx + count - _size; // value 자리 init 개수
                const size_t tail_cnt = _size - idx;

                construct_guard<Allocator> guard(_allocator, &_data[_size]);
                try
                {
                    // 1) value를 uninitialized 영역에 새로 construct
                    // [size, size + extra_cnt)
                    uninitialized_fill(&_data[_size], value, extra_cnt, _allocator);
                    guard.bump_n(extra_cnt);

                    // 2) 기존 값을 uninitialized 영역에 shift
                    // [idx, size) -> [idx + count, size + count)
                    uninitialized_copy(&_data[idx], &_data[_size], &_data[idx + count], _allocator);
                    guard.bump_n(_size - idx);

                    // 3) value를 initialized 영역에 assignment
                    // [idx, size)
                    for (size_t i = 0; i < tail_cnt; ++i)
                        _data[idx + i] = value;
                }
                catch (...)
                {
                    // guard의 소멸자가 rollback 수행
                    // 실행 중이던 부분은 내부 guard가 rollback함
                    throw;
                }
                guard.release();
            }
            _size += count;
            return begin() + idx;
        }

        // 범위 삽입
        // enable_if로 혼동 방지
        /*
            iterator의 tag에 따라 분기 처리
            (1) Input iterator
                - distance 계산 & multi-pass 둘다 불가능
                - 하나씩 insert() 해야함.
                - insert()는 O(n)이고 이것을 n번 실행, 즉 O(n^2)
            (2) Forward & Bidirectional iterator
                - distance 계산 O(n)에 가능
                - 이후 O(n) 처리 가능
            (3) Random Access
                - distance 계산 O(1)에 가능
                - 이후 O(n) 처리 가능
        */
        template <class InputIt>
        iterator insert(const_iterator pos, InputIt first, InputIt last,
                        typename ft::enable_if<!ft::is_integral<InputIt>::value, void>::type * = 0)
        {
            if (first == last)
                return const_cast<iterator>(pos);

            typedef typename ft::iterator_traits<InputIt>::iterator_category category;
            // category()는 category 타입으로 만드는 임시 객체 (오버로딩 분기용)
            return _range_insert(pos, first, last, category());
        }

        // Input iterator (single-pass)
        // pos는 random access 보장됨
        template <class InputIt>
        iterator _range_insert(const_iterator cpos, InputIt first, InputIt last,
                               std::input_iterator_tag)
        {
            const size_type idx = cpos - begin();
            iterator        pos;

            // single-pass라서 distance 계산이 불가 -> 하나씩 insert
            pos = begin() + idx;
            for (; first != last; ++first)
            {
                // insert 내부에서 reserve 호출시 기존 itertor는 무효화되므로,
                // 반드시 리턴을 받아 사용
                pos = insert(pos, *first);
                ++pos;
            }
            return begin() + idx;
        }

        // Forward & Bidirectional iterator (double-pass)
        // 상속 구조에 의해, Bidirectional은 이 함수로 dispatch
        // Random Access도 여기서 처리 (std::distance가 내부적으로 dispatch)
        template <class InputIt>
        iterator _range_insert(const_iterator cpos, InputIt first, InputIt last,
                               std::forward_iterator_tag)
        {
            const size_type idx = cpos - begin();
            const size_type count = std::distance(first, last);

            if (count == 0)
                return begin() + idx;
            if (_size + count > _capacity)
                reserve(_grow_capacity_to(_size + count));

            if (idx == _size) // 1. idx == size
            {
                uninitialized_copy(first, last, &_data[idx], _allocator);
            }
            else if (idx + count <= _size) // 2. idx + count < _size
            {
                // 1) 맨 뒤쪽 count개를 uninitialized 영역에 생성
                uninitialized_copy(&_data[_size - count], &_data[_size], &_data[_size], _allocator);

                /*
                    - 복사 대입(operator=)이 noexcept인게 보장되어 있지 않음
                    - 포인터처럼 접근이 불가한 iterator일 수도 있으므
                    => assignment 연산에 대해서도 rollback 처리 필요
                */
                try
                {
                    // 2) [idx, size - count)를 [idx + count, size)로 shift
                    for (size_t i = _size - count; i > idx; --i)
                        _data[i + count - 1] = _data[i - 1];

                    // 3) [idx, idx + count)를 range로 채우기
                    size_t i = idx;
                    for (; first != last; ++first, ++i)
                        _data[i] = *first;
                }
                catch (...)
                {
                    destroy_range(_allocator, _data + _size, _data + _size + count);
                    throw;
                }
            }
            else // 3. idx < _size && idx + count >= _size
            {
                // [idx, size) : initialized
                // [size, idx + count) : uninitialized
                // [idx + count, size + count) : uninitialized

                const size_t tail_cnt = _size - idx;          // 원본의 꼬리 원소들 개수
                const size_t extra_cnt = idx + count - _size; // tail 이후의 추가 분량

                // 새롭게 init이 필요한 개수 = count
                // count = (tail_cnt + extra_cnt)
                // tail_cnt: [idx, size)의 거리
                // extra_cnt: [size, idx + count)의 거리

                InputIt mid = first;
                std::advance(mid, tail_cnt);
                // forward iterator이므로 앞으로 나아가는 것만 가능
                // mid는 말 그대로 중간 지점

                construct_guard<Allocator> guard(_allocator, &_data[_size]);
                try
                {
                    // 1) [first + tail_cnt, last)를 uninitialized 영역에 새로 construct
                    // [size, size + extra_cnt)
                    uninitialized_copy(mid, last, &_data[_size], _allocator);
                    guard.bump_n(extra_cnt);

                    // 2) 기존 값을 uninitialized 영역에 shift
                    // [idx, size) -> [idx + count, size + count)
                    uninitialized_copy(&_data[idx], &_data[_size], &_data[idx + count], _allocator);
                    guard.bump_n(_size - idx);

                    // 3) [first, mid)를 initialized 영역에 assignment
                    // [idx, size)
                    size_t i = idx;
                    for (; first != mid; ++first, ++i)
                        _data[i] = *first;
                }
                catch (...)
                {
                    // guard의 소멸자가 rollback 수행
                    // 실행 중이던 부분은 내부 guard가 rollback함
                    throw;
                }
                guard.release();
            }
            _size += count;
            return begin() + idx;
        }

        /*
            - STL 내부 iterator는 어차피 포인터임.
                => iterator는 값이 아니라 offset으로 다루는게 안전하다
            - iterator invalidation(반복자 무효화) 규칙:
                삭제 구간 이후의 모든 iterator는 무효화된다.
        */
        // erases elements
        // 단일 삭제
        iterator erase(iterator pos)
        {
            const size_type idx = pos - begin();

            if (pos == end())
                return end();

            // 삭제 대상 위치 뒤쪽의 원소들을 앞으로 옮김
            // 왼쪽으로 shift: 바로 뒷 원소를 대입해나가기
            for (size_type i = idx; i < _size - 1; ++i)
                _data[i] = _data[i + 1];

            // 마지막 원소 파괴
            _allocator.destroy(_data + _size - 1);
            --_size;

            // iterator 원칙상, 삭제된 원소는 invalidated된 것으로 취급
            // 따라서 주소가 같을지라도 새 iterator를 반환해야한다
            return begin() + idx;
        }

        // 범위 삭제
        iterator erase(iterator first, iterator last)
        {
            if (first == last)
                return first;

            const size_type idx = first - begin(); // 삭제 시작 위치
            const size_type count = last - first;

            // 1 2 3 4 5 6 7
            // 1 2 x x x 6 7
            // 1 2 6 7

            // tail을 왼쪽으로 count만큼 당김
            // [idx + count, _size) -> [idx, _size - count)
            // 왼쪽으로 shift: 바로 뒷 원소를 대입해나가기
            for (size_type i = idx; i + count < _size; ++i)
                _data[i] = _data[i + count];

            destroy_range(_allocator, &_data[_size - count], &_data[_size]);
            _size -= count;
            return begin() + idx;
        }

        // changes the number of elements stored
        void resize(size_type count)
        {
            if (_size > count)
            {
                // remove
                destroy_range(_allocator, _data + count, _data + _size);
            }
            else if (_size < count)
            {
                // add copies of T() (or value)
                if (_capacity < count)
                    reserve(_grow_capacity_to(count));
                uninitialized_fill(_data + _size, value_type(), count - _size, _allocator);
            }
            _size = count;
        }
        void resize(size_type count, const value_type &value)
        {
            if (_size > count)
            {
                // remove
                destroy_range(_allocator, _data + count, _data + _size);
            }
            else if (_size < count)
            {
                // add copies of T() (or value)
                if (_capacity < count)
                    reserve(_grow_capacity_to(count));
                uninitialized_fill(_data + _size, value, count - _size, _allocator);
            }
            _size = count;
        }

        // -------------------- Iterators -------------------- //
        iterator       begin() { return _data; }
        const_iterator begin() const { return _data; }
        iterator       end() { return _data + _size; }
        const_iterator end() const { return _data + _size; }

        /*
        타입명과 생성자명이 같지만, 컴파일러는 문맥상으로 알맞게 판별한다
            - 반환 타입은 선언 문맥으로 해석
            - return 뒤의 괄호 호출은 값 생성(표현식)으로 해석
        아래의 reverse_iterator 그리고 const reverse_iterator가 그 예시
        */
        reverse_iterator       rbegin() { return reverse_iterator(end()); }
        const_reverse_iterator rbegin() const { return const_reverse_iterator(end()); }
        reverse_iterator       rend() { return reverse_iterator(begin()); }
        const_reverse_iterator rend() const { return const_reverse_iterator(begin()); }

      private:
        allocator_type _allocator;
        pointer        _data;
        size_type      _size;
        size_type      _capacity;

        // 최소 needed 이상이 되는 capacity를 반환
        size_type _grow_capacity_to(size_type needed) const
        {
            size_type cap = (_capacity == 0 ? 1 : _capacity);

            // 이미 충분하면 그대로
            if (cap >= needed)
                return cap;
            // grow
            while (cap < needed)
            {
                // 1.5x with progress guarantee
                // inc = increment
                size_type inc = cap / 2;
                cap = (inc == 0 ? cap + 1 : cap + inc);
            }
            return cap;
        }
    };

    // -------------------- Non-member operators -------------------- //

    template <class T, class Allocator>
    bool operator==(const vector<T, Allocator> &lhs, const vector<T, Allocator> &rhs)
    {
        if (lhs.size() != rhs.size())
            return false;
        return ft::equal(lhs.begin(), lhs.end(), rhs.begin());
    }

    template <class T, class Allocator>
    bool operator!=(const vector<T, Allocator> &lhs, const vector<T, Allocator> &rhs)
    {
        return !(lhs == rhs);
    }

    template <class T, class Allocator>
    bool operator<(const vector<T, Allocator> &lhs, const vector<T, Allocator> &rhs)
    {
        return ft::lexicographical_compare(lhs.begin(), lhs.end(), rhs.begin(), rhs.end());
    }

    template <class T, class Allocator>
    bool operator<=(const vector<T, Allocator> &lhs, const vector<T, Allocator> &rhs)
    {
        return !(rhs < lhs);
    }

    template <class T, class Allocator>
    bool operator>(const vector<T, Allocator> &lhs, const vector<T, Allocator> &rhs)
    {
        return (rhs < lhs);
    }

    template <class T, class Allocator>
    bool operator>=(const vector<T, Allocator> &lhs, const vector<T, Allocator> &rhs)
    {
        return !(lhs < rhs);
    }
} // namespace ft

#endif // FT_VECTOR_H

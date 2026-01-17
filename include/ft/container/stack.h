#ifndef FT_STACK_H
#define FT_STACK_H

#include <cstddef>
#include "vector.h"

namespace ft
{
    // =========================================================================
    // stack (container adaptor)
    // 기본 컨테이너: ft::vector<T>
    // =========================================================================
    template <class T, class Container = ft::vector<T> >
    class stack
    {
      public:
        typedef T           value_type;
        typedef Container   container_type;
        typedef std::size_t size_type;

      protected:
        container_type c;

      public:
        // ---------------- Constructors / Destructor ---------------- //
        explicit stack(const container_type &cont = container_type()) : c(cont) {}

        stack(const stack &other) : c(other.c) {}

        ~stack() {}

        stack &operator=(const stack &other)
        {
            if (this != &other)
                c = other.c;
            return *this;
        }

        // ---------------- Element access ---------------- //
        value_type       &top() { return c.back(); }
        const value_type &top() const { return c.back(); }

        // ---------------- Capacity ---------------- //
        bool      empty() const { return c.empty(); }
        size_type size() const { return static_cast<size_type>(c.size()); }

        // ---------------- Modifiers ---------------- //
        void push(const value_type &val) { c.push_back(val); }
        void pop() { c.pop_back(); }

        // 비교 연산자들이 protected 멤버 c에 접근할 수 있도록 friend 선언
        template <class U, class Cont>
        friend bool operator==(const stack<U, Cont> &lhs, const stack<U, Cont> &rhs);

        template <class U, class Cont>
        friend bool operator<(const stack<U, Cont> &lhs, const stack<U, Cont> &rhs);
    };

    // ---------------- Comparison operators implementations ---------------- //
    template <class T, class Container>
    bool operator==(const stack<T, Container> &lhs, const stack<T, Container> &rhs)
    {
        return lhs.c == rhs.c;
    }

    template <class T, class Container>
    bool operator!=(const stack<T, Container> &lhs, const stack<T, Container> &rhs)
    {
        return !(lhs == rhs);
    }

    template <class T, class Container>
    bool operator<(const stack<T, Container> &lhs, const stack<T, Container> &rhs)
    {
        return lhs.c < rhs.c;
    }

    template <class T, class Container>
    bool operator<=(const stack<T, Container> &lhs, const stack<T, Container> &rhs)
    {
        return !(rhs < lhs);
    }

    template <class T, class Container>
    bool operator>(const stack<T, Container> &lhs, const stack<T, Container> &rhs)
    {
        return rhs < lhs;
    }

    template <class T, class Container>
    bool operator>=(const stack<T, Container> &lhs, const stack<T, Container> &rhs)
    {
        return !(lhs < rhs);
    }

} // namespace ft

#endif // FT_STACK_H

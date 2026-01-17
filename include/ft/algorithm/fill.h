#ifndef FT_FILL_H
#define FT_FILL_H

// std::fill을 참조

namespace ft
{
    template <class ForwardIt, class T>
    void fill(ForwardIt first, ForwardIt last, const T &value)
    {
        while (first != last)
            *(first++) = value;
    }

    template <class OutputIt, class T>
    OutputIt fill_n(OutputIt first, size_t count, const T &value)
    {
        for (; count > 0; --count, ++first)
            *first = value;
        return first;
    }
} // namespace ft

#endif

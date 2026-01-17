#pragma once
#include <sstream>   // std::ostringstream
#include <stdexcept> // std::out_of_range

namespace ft
{
    inline void throw_out_of_range_index(const char *container, const char *func, std::size_t n,
                                         std::size_t size)
    {
        std::ostringstream oss;
        oss << container << "::" << "_M" << func << ": __n (which is " << n
            << ") >= this->size() (which is " << size << ")";
        throw std::out_of_range(oss.str());
    }
} // namespace ft

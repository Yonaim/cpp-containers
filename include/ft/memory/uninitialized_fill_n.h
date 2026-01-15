#ifndef FT_UNINITIALIZED_FILL_N_H
#define FT_UNINITIALIZED_FILL_N_H

/*
    - uninitialized = 메모리 확보는 됐고, 객체는 아직 생성 안 됨
    - uninitialized_fill_n = 그 자리에서 count개를 생성해서 채움
*/

//  Copies value to an uninitialized memory area first + [​0​, count)
namespace ft
{
    // NoThrowForwardIt: exception을 던지지 않을 것을 기대
    template <class NoThrowForwardIt, class size_t, class T>
    NoThrowForwardIt uninitialized_fill_n(NoThrowForwardIt first, size_t count, const T &value)
    {
        size_t i = 0;
        try
        {
            while (i < count)
            {
                ::new (first + i) T(value);
                i++;
            }
        }
        catch (...)
        {
            for (size_t j = 0; j < i; ++j)
                (first + i)->~T();
            throw;
        }
        return first;
    }
} // namespace ft

#endif

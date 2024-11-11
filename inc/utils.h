#ifndef __UTILS_H__
#define __UTILS_H__

#include <algorithm>

#include "configuration.h"

#define INT_DIVIDE_CEILING(x,y) (((x) + (y) - 1) / (y))
#define INT_DIVIDE_ROUND(x,y) (((x) + ((y) / 2)) / (y))
// Just for completeness...
#define INT_DIVIDE_FLOOR(x,y) ((x)/(y))

#if SHOW_DEBUG_MESSAGES && !defined(UNITTEST)
    #include <stdio.h>
    #define DEBUG_PRINT(...) printf (__VA_ARGS__)
#else
    #define DEBUG_PRINT(...)
#endif

template <typename T>
inline T limit_value(T value, T min, T max)
{
    return std::min(std::max(value, min), max);
}

#endif // __UTILS_H__

#ifndef __STL_VECTOR__
#define __STL_VECTOR__

#include <vector>
#include "tAllocator.h"

template <typename T>
class tSTLVector: public std::vector<T, tAllocator<T>> {};

#endif

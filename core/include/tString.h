#ifndef __TSTRING_H__
#define __TSTRING_H__

#include <string>
#include "tAllocator.h"

typedef std::basic_string<_char_t, std::char_traits<_char_t>, tAllocator<_char_t>> tString;

#endif

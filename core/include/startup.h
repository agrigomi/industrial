#ifndef __STARTUP_H__
#define __STARTUP_H__

#include <vector>
#include "dtype.h"

typedef struct {
	void *ibase;
	_u32 refc;
}_base_array_t;

typedef std::vector<_base_array_t> _base_vector_t;


#endif


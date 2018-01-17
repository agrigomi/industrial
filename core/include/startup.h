#ifndef __STARTUP_H__
#define __STARTUP_H__

#include <vector>
#include "iRepository.h"

typedef struct {
	iBase *pi_base;
	_u32  ref_cnt;
}_base_entry_t;

typedef std::vector<_base_entry_t> _base_vector_t;

extern "C" {
void init(iRepository *pi_repo=0);
_base_vector_t *get_base_vector(void);
};

#endif


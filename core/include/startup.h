#ifndef __STARTUP_H__
#define __STARTUP_H__

#include <vector>
#include "iRepository.h"

#define _EXPORT_ __attribute__ ((visibility ("default")))
#define _LOCAL_ __attribute__ ((visibility ("hidden")))

#define ST_INITIALIZED	(1<<0)
#define ST_DISABLED	(1<<1)

typedef struct {
	iBase *pi_base;
	_u32  ref_cnt;
	_u8   state;
}_base_entry_t;

typedef std::vector<_base_entry_t> _base_vector_t;

extern "C" {
#ifdef _CORE_
_err_t init(int argc, char *argv[]);
#else
_err_t init(iRepository *pi_repo=0);
#endif
_base_vector_t *get_base_vector(void);
iRepository *get_repository(void);
};

#endif


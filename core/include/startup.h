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

extern "C" {
#ifdef _CORE_
_err_t init(int argc, char *argv[]);
#else
_err_t init(iRepository *pi_repo=0);
#endif
_base_entry_t *get_base_array(void);
_u32 get_base_array_limit(void);
_u32 get_base_array_count(void);
iRepository *get_repository(void);
};

#define IMPLEMENT_BASE_ARRAY(limit) \
	_LOCAL_ _base_entry_t _g_base_arry_[limit]; \
	_LOCAL_ _u32 _base_array_limit_ = limit; \
	_LOCAL_ _u32 _base_array_count_ = 0;

extern _base_entry_t _g_base_array_[];
extern _u32 _base_array_limit_;
extern _u32 _base_array_count_;

#endif


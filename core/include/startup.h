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
_base_entry_t *get_base_array(_u32 *count, _u32 *limit);
void add_base_entry(_base_entry_t *entry);
iRepository *get_repository(void);
};

#define IMPLEMENT_BASE_ARRAY(limit) \
	_LOCAL_ _base_entry_t _g_base_array_[limit]; \
	_LOCAL_ _u32 _base_array_limit_ = limit; \
	_LOCAL_ _u32 _base_array_count_ = 0; \
	_EXPORT_ _base_entry_t *get_base_array(_u32 *c, _u32 *l) { \
		*c = _base_array_count_; \
		*l = _base_array_limit_; \
		return _g_base_array_; \
	} \
	_EXPORT_ void add_base_entry(_base_entry_t *e) { \
		if(_base_array_count_ < _base_array_limit_) { \
			_g_base_array_[_base_array_count_].pi_base = e->pi_base; \
			_g_base_array_[_base_array_count_].ref_cnt = e->ref_cnt; \
			_g_base_array_[_base_array_count_].state = e->state; \
			_base_array_count_++; \
		} \
	}
#endif


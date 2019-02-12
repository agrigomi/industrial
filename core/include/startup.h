#ifndef __STARTUP_H__
#define __STARTUP_H__

#include <stdio.h>
#include <signal.h>
#include "iRepository.h"

#define _EXPORT_ __attribute__ ((visibility ("default")))
#define _LOCAL_ __attribute__ ((visibility ("hidden")))

extern "C" {
#ifdef _CORE_
typedef void _sig_action_t(int signum, siginfo_t *info, void *);

_err_t init(int argc, char *argv[]);
void uninit(void);
void dump_stack(void);
void handle(int sig, _sig_action_t *pf_action);
#else
_err_t init(iRepository *pi_repo);
#endif
_base_entry_t *get_base_array(_u32 *count, _u32 *limit);
void add_base_entry(_base_entry_t *entry);
iRepository *get_repository(void);
};

#define IMPLEMENT_BASE_ARRAY(ext_name, limit) \
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
		} else \
			printf("No enough space in base array '%s'!\n", ext_name); \
	}
#endif


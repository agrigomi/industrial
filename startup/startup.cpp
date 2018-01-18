#include <string.h>
#include "startup.h"
#include "iMemory.h"

// global pointer to repository
_LOCAL_ iRepository *_gpi_repo_ = 0;

static _base_vector_t _g_base_vector_;

void _LOCAL_ register_object(iBase *pi_base) {
	_base_entry_t be = {pi_base, 0, 0};
	_g_base_vector_.push_back(be);
}

typedef struct {
	_cstr_t iname;
	iBase   *pi_base;
}_early_init_t;

_err_t _EXPORT_ init(iRepository *pi_repo) {
	_err_t r = ERR_UNKNOWN;
	_gpi_repo_ = pi_repo;
#ifdef _CORE_
	_early_init_t ei[]= {
		{I_REPOSITORY,	0},
		{I_HEAP,	0},
		{0,		0}
	};

	_base_vector_t::iterator i = _g_base_vector_.begin();
	while(i != _g_base_vector_.end()) {
		_object_info_t oinfo;
		if(i->pi_base) {
			i->pi_base->object_info(&oinfo);
			for(_u32 j = 0; ei[j].iname; j++) {
				if(strcmp(oinfo.iname, ei[j].iname) == 0) {
					ei[j].pi_base = i->pi_base;
					break;
				}
			}
		}
		//...
		i++;
	}
#endif
	return r;
}

_base_vector_t _EXPORT_ *get_base_vector(void) {
	return &_g_base_vector_;
}

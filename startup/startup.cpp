#include "startup.h"

// global pointer to repository
_LOCAL_ iRepository *_gpi_repo_ = 0;

static _base_vector_t _g_base_vector_;

void _LOCAL_ register_object(iBase *pi_base) {
	_base_entry_t be;
	be.pi_base = pi_base;
	be.ref_cnt = 0;
	be.state = 0;
	_g_base_vector_.push_back(be);
}

_err_t _EXPORT_ init(iRepository *pi_repo) {
	_err_t r = ERR_UNKNOWN;
	_gpi_repo_ = pi_repo;
#ifdef _CORE_
	_base_vector_t::iterator i = _g_base_vector_.begin();
	while(i != _g_base_vector_.end()) {
		//...
		i++;
	}
#endif
	return r;
}

_base_vector_t _EXPORT_ *get_base_vector(void) {
	return &_g_base_vector_;
}

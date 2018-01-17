#include "startup.h"
#include "iRepository.h"

// global pointer to repository
iRepository *_gpi_repo_ = 0;

static _base_vector_t _g_base_vector_;

void _LOCAL_ _register_object_(iBase *pi_base) {
}

void _EXPORT_ init(iRepository *pi_repo) {
	_gpi_repo_ = pi_repo;
#ifdef _CORE_
	//...
#endif
}

_base_vector_t _EXPORT_ *get_base_vector(void) {
	return &_g_base_vector_;
}

#include "startup.h"
#include "iRepository.h"

#define _EXPORT_ __attribute__ ((visibility ("default")))
#define _LOCAL_ __attribute__ ((visibility ("hidden")))

// global pointer to repository
iRepository *_gpi_repo_ = 0;

_base_vector_t _g_base_vector_;

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

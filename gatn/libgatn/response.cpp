#include "iGatn.h"
#include "private.h"

void response::var(_cstr_t name, _cstr_t value) {
	if(mpi_httpc)
		mpi_httpc->res_var(name, value);
}

_u32 response::write(void *data, _u32 size) {
	_u32 r = 0;

	//...

	return r;
}

_u32 response::end(_u16 response_code, void *data, _u32 size) {
	_u32 r = 0;

	//...

	return r;
}

void response::destroy(void) {
	//...
}

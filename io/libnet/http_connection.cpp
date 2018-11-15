#include "private.h"

bool cHttpConnection::object_ctl(_u32 cmd, void *arg, ...) {
	bool r = false;

	switch(cmd) {
		case OCTL_INIT: {
			iRepository *pi_repo = (iRepository *)arg;

			mp_sio = NULL;
			if((mpi_str = (iStr *)pi_repo->object_by_iname(I_STR, RF_ORIGINAL))) {
				r = true;
			}
		} break;
		case OCTL_UNINIT: {
			iRepository *pi_repo = (iRepository *)arg;

			pi_repo->object_release(mpi_str);
			_close();
			r = true;
		} break;
	}

	return r;
}

bool cHttpConnection::_init(cSocketIO *p_sio) {
	bool r = false;

	if(p_sio && (r = p_sio->alive())) {
		mp_sio = p_sio;
		// use non blocking mode
		mp_sio->blocking(false);
	}

	return r;
}

void cHttpConnection::_close(void) {
	if(mp_sio)
		mp_sio->_close();
}

bool cHttpConnection::alive(void) {
	bool r = false;

	if(mp_sio)
		r = mp_sio->alive();

	return r;
}

// copies value of http header variable into buffer
bool cHttpConnection::copy_value(_cstr_t vname, _str_t buffer, _u32 sz_buffer) {
	bool r = false;

	//...

	return r;
}

static cHttpConnection _g_httpc_;

#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include "private.h"

bool cSocketIO::object_ctl(_u32 cmd, void *arg, ...) {
	bool r = false;

	switch(cmd) {
		case OCTL_INIT:
			m_socket = 0;
			r = true;
			break;
		case OCTL_UNINIT:
			_close();
			r = true;
			break;
	}

	return r;
}

void cSocketIO::_init(struct sockaddr_in *psa, _s32 socket) {
	m_socket = socket;
	memcpy(&m_sa, psa, sizeof(struct sockaddr_in));
}

void cSocketIO::_close(void) {
	if(m_socket) {
		close(m_socket);
		m_socket = 0;
	}
}

_u32 cSocketIO::read(void *data, _u32 size) {
	_u32 r = 0;

	if(m_socket) {
		socklen_t sl = sizeof(m_sa);
		_s32 _r = recvfrom(m_socket, data, size, 0, (struct sockaddr*)&m_sa, &sl);
		if(_r > 0)
			r = _r;
	}

	return r;
}

_u32 cSocketIO::write(void *data, _u32 size) {
	_u32 r = 0;
	//...
	return r;
}

void cSocketIO::blocking(bool mode) { /* blocking or nonblocking IO */
}

static cSocketIO _g_socket_io_;

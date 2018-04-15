#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#include "private.h"

bool cSocketIO::object_ctl(_u32 cmd, void *arg, ...) {
	bool r = false;

	switch(cmd) {
		case OCTL_INIT:
			m_socket = 0;
			memset(&m_sa, 0, sizeof(m_sa));
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
	m_addrlen = sizeof(m_sa);
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
		_s32 _r = recvfrom(m_socket, data, size, 0, (struct sockaddr*)&m_sa, &m_addrlen);
		if(_r > 0)
			r = _r;
	}

	return r;
}

_u32 cSocketIO::write(void *data, _u32 size) {
	_u32 r = 0;

	if(m_socket) {
		_s32 _r = sendto(m_socket, data, size, 0, (struct sockaddr*)&m_sa, sizeof(m_sa));
		if(_r > 0)
			r = _r;
	}

	return r;
}

void cSocketIO::blocking(bool mode) { /* blocking or nonblocking IO */
	if(m_socket) {
		_s32 flags = fcntl(m_socket, F_GETFL, 0);
		if(flags != -1) {
			flags = (mode) ? (flags & ~O_NONBLOCK) : (flags | O_NONBLOCK);
			fcntl(m_socket, F_SETFL, flags);
		}
	}
}

static cSocketIO _g_socket_io_;

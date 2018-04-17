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
			mp_clientaddr = mp_serveraddr = 0;
			r = true;
			break;
		case OCTL_UNINIT:
			_close();
			r = true;
			break;
	}

	return r;
}

void cSocketIO::_init(struct sockaddr_in *p_saddr, // server addr
			struct sockaddr_in *p_caddr, // client addr
			_s32 socket, _u8 mode) {
	m_socket = socket;
	m_mode = mode;
	mp_clientaddr = p_caddr;
	mp_serveraddr = p_saddr;
}

struct sockaddr_in *cSocketIO::serveraddr(void) {
	return mp_serveraddr;
}

struct sockaddr_in *cSocketIO::clientaddr(void) {
	return mp_clientaddr;
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
		switch(m_mode) {
			case SOCKET_IO_UDP: {
				socklen_t addrlen = sizeof(struct sockaddr_in);
				_s32 _r = recvfrom(m_socket, data, size, 0,
						(struct sockaddr *)mp_clientaddr,
						(mp_clientaddr)?&addrlen:0);
				if(_r > 0)
					r = _r;
			} break;
			case SOCKET_IO_TCP: {
				//...
			} break;
		}
	}

	return r;
}

_u32 cSocketIO::write(void *data, _u32 size) {
	_u32 r = 0;

	if(m_socket) {
		switch(m_mode) {
			case SOCKET_IO_UDP: {
				_s32 _r = sendto(m_socket, data, size, 0,
						(struct sockaddr *)mp_clientaddr,
						sizeof(struct sockaddr_in));
				if(_r > 0)
					r = _r;
			} break;
			case SOCKET_IO_TCP: {
				//...
			} break;
		}
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

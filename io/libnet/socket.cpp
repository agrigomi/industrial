#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#include "private.h"
#include "iRepository.h"
#include "iMemory.h"

bool cSocketIO::object_ctl(_u32 cmd, void *arg, ...) {
	bool r = false;

	switch(cmd) {
		case OCTL_INIT:
			m_socket = 0;
			memset(&m_serveraddr, 0, sizeof(struct sockaddr_in));
			memset(&m_clientaddr, 0, sizeof(struct sockaddr_in));
			r = true;
			break;
		case OCTL_UNINIT:
			_close();
			r = true;
			break;
	}

	return r;
}

bool cSocketIO::_init(struct sockaddr_in *p_saddr, // server addr
			struct sockaddr_in *p_caddr, // client addr
			_s32 socket, _u8 mode, SSL_CTX *p_ssl_cxt) {
	bool r = false;

	m_socket = socket;
	m_mode = mode;
	m_alive = true;

	if(p_saddr)
		memcpy(&m_serveraddr, p_saddr, sizeof(struct sockaddr_in));
	if(p_caddr)
		memcpy(&m_clientaddr, p_caddr, sizeof(struct sockaddr_in));

	_s32 opt = 1;
	setsockopt(m_socket, SOL_SOCKET, SO_KEEPALIVE, &opt, sizeof(opt));

	if((m_mode == SOCKET_IO_SSL_SERVER || m_mode == SOCKET_IO_SSL_CLIENT) && p_ssl_cxt) {
		if((mp_cSSL = SSL_new(p_ssl_cxt))) {
			SSL_set_fd(mp_cSSL, m_socket);
			if(m_mode == SOCKET_IO_SSL_SERVER) {
				if(SSL_accept(mp_cSSL) > 0)
					r = true;
			} else {
				// SSL client
				if(SSL_connect(mp_cSSL) > 0)
					r = true;
			}
		}
	} else
		r = true;

	return r;
}

struct sockaddr_in *cSocketIO::serveraddr(void) {
	return &m_serveraddr;
}

struct sockaddr_in *cSocketIO::clientaddr(void) {
	return &m_clientaddr;
}

void cSocketIO::_close(void) {
	if(m_socket) {
		if(::close(m_socket) >= 0) {
			m_socket = 0;

			if(mp_cSSL)
				SSL_free(mp_cSSL);
		}
	}
}

_u32 cSocketIO::read(void *data, _u32 size) {
	_u32 r = 0;

	if(m_socket && m_alive && size) {
		switch(m_mode) {
			case SOCKET_IO_UDP: {
				socklen_t addrlen = sizeof(struct sockaddr);
				_s32 _r = recvfrom(m_socket, data, size, 0,
						(struct sockaddr *)&m_clientaddr,
						&addrlen);
				if(_r > 0)
					r = _r;
				else {
					if(_r == 0)
						m_alive = false;
				}
			} break;
			case SOCKET_IO_TCP: {
				_s32 _r = ::read(m_socket, data, size);
				if(_r > 0)
					r = _r;
				else {
					if(_r == 0)
						m_alive = false;
				}
			} break;
			case SOCKET_IO_SSL_SERVER:
			case SOCKET_IO_SSL_CLIENT: {
				_u32 _r = SSL_read(mp_cSSL, data, size);
				if(_r > 0)
					r = _r;
				else {
					if(SSL_get_error(mp_cSSL, _r) == SSL_ERROR_ZERO_RETURN)
						m_alive = false;
				}
			} break;
		}
	}

	return r;
}

_u32 cSocketIO::write(const void *data, _u32 size) {
	_u32 r = 0;

	if(m_socket && m_alive && size) {
		switch(m_mode) {
			case SOCKET_IO_UDP: {
				_s32 _r = sendto(m_socket, data, size, 0,
						(struct sockaddr *)&m_clientaddr,
						sizeof(struct sockaddr));
				if(_r > 0)
					r = _r;
				else
					m_alive = false;
			} break;
			case SOCKET_IO_TCP: {
				_s32 _r = ::send(m_socket, data, size, MSG_NOSIGNAL);
				if(_r > 0)
					r = _r;
				else
					m_alive = false;
			} break;
			case SOCKET_IO_SSL_SERVER:
			case SOCKET_IO_SSL_CLIENT: {
				_u32 _r = SSL_write(mp_cSSL, data, size);
				if(_r > 0)
					r = _r;
				else {
					if(SSL_get_error(mp_cSSL, _r) == SSL_ERROR_ZERO_RETURN)
						m_alive = false;
				}
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

bool cSocketIO::alive(void) {
	return m_alive;
}

_u32 cSocketIO::peer_ip(void) {
	_u32 r = 0;

	if(m_socket) {
		struct sockaddr_storage addr;
		struct sockaddr_in *s = (struct sockaddr_in *)&addr;
		socklen_t _len = sizeof addr;

		getpeername(m_socket, (struct sockaddr*)&addr, &_len);
		r = s->sin_addr.s_addr;
	}

	return r;
}

bool cSocketIO::peer_ip(_str_t strip, _u32 len) {
	bool r = false;

	if(m_socket) {
		struct sockaddr_storage addr;
		struct sockaddr_in *s = (struct sockaddr_in *)&addr;
		socklen_t _len = sizeof addr;

		getpeername(m_socket, (struct sockaddr*)&addr, &_len);
		inet_ntop(AF_INET, &s->sin_addr, strip, len);
	}

	return r;
}

static cSocketIO _g_socket_io_;

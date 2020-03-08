#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#include "private.h"

bool cTCPServer::_init(_u32 port, SSL_CTX *ssl_context) {
	bool r = false;

	if((m_server_socket = socket(AF_INET, SOCK_STREAM, 0)) > 0) {
		m_port = port;
		mp_sslcxt = ssl_context;

		_s32 opt = 1;
		setsockopt(m_server_socket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
		setsockopt(m_server_socket, SOL_SOCKET, SO_REUSEPORT, &opt, sizeof(opt));
		setsockopt(m_server_socket, SOL_SOCKET, SO_KEEPALIVE, &opt, sizeof(opt));
		opt = 10;
		setsockopt(m_server_socket, SOL_TCP, TCP_KEEPIDLE, &opt, sizeof(opt));
		opt = 3;
		setsockopt(m_server_socket, SOL_TCP, TCP_KEEPCNT, &opt, sizeof(opt));

		m_serveraddr.sin_family = AF_INET;
		m_serveraddr.sin_addr.s_addr = htonl(INADDR_ANY);
		m_serveraddr.sin_port = htons((unsigned short)port);

		if(bind(m_server_socket, (struct sockaddr *)&m_serveraddr, sizeof(m_serveraddr)) >= 0) {
			if(::listen(m_server_socket, 10) >= 0)
				r = true;
		}

		if(!r) {
			::close(m_server_socket);
			m_server_socket = 0;
		}
	}

	return r;
}

void cTCPServer::_close(void) {
	if(m_server_socket) {
		_s32 err = 1;
		socklen_t len = sizeof(err);

		getsockopt(m_server_socket, SOL_SOCKET, SO_ERROR, (char *)&err, &len);

		shutdown(m_server_socket, SHUT_RDWR);
		::close(m_server_socket);

		m_server_socket = 0;
	}
}

bool cTCPServer::object_ctl(_u32 cmd, void *arg, ...) {
	bool r = false;

	switch(cmd) {
		case OCTL_INIT: {
			iRepository *pi_repo = (iRepository *)arg;
			mpi_heap = (iHeap *)pi_repo->object_by_iname(I_HEAP, RF_ORIGINAL);
			memset(&m_serveraddr, 0, sizeof(struct sockaddr_in));
			m_server_socket = 0;
			mp_sslcxt = 0;
			if((m_hsio = pi_repo->handle_by_cname(CLASS_NAME_SOCKET_IO)))
				r = true;
		} break;
		case OCTL_UNINIT: {
			_close();
			iRepository *pi_repo = (iRepository *)arg;
			pi_repo->object_release(mpi_heap);
			r = true;
		} break;
	}

	return r;
}

iSocketIO *cTCPServer::listen(void) {
	iSocketIO *r = 0;

	if(m_server_socket) {
		struct sockaddr_in caddr;

		memset(&caddr, 0, sizeof(struct sockaddr_in));

		socklen_t addrlen = sizeof(struct sockaddr_in);
		_s32 connect_socket = accept(m_server_socket, (struct sockaddr *)&caddr, &addrlen);

		if(connect_socket > 0) {
			cSocketIO *psio = (cSocketIO *)_gpi_repo_->object_by_handle(m_hsio, RF_CLONE|RF_NONOTIFY);
			if(psio) {
				if(psio->_init(0, &caddr, connect_socket,
						(mp_sslcxt) ? SOCKET_IO_SSL_SERVER : SOCKET_IO_TCP, mp_sslcxt))
					r = psio;
				else
					/* we assume that socket I/O object should close socket handle */
					_gpi_repo_->object_release(psio);
			} else
				::close(connect_socket);
		}
	}

	return r;
}

void cTCPServer::blocking(bool mode) { /* blocking or nonblocking IO */
	if(m_server_socket) {
		_s32 flags = fcntl(m_server_socket, F_GETFL, 0);
		if(flags != -1) {
			flags = (mode) ? (flags & ~O_NONBLOCK) : (flags | O_NONBLOCK);
			fcntl(m_server_socket, F_SETFL, flags);
		}
	}
}

void cTCPServer::close(iSocketIO *p_io) {
	cSocketIO *pcsio = dynamic_cast<cSocketIO *>(p_io);

	if(pcsio) {
		pcsio->_close();
		_gpi_repo_->object_release(p_io);
	}
}

static cTCPServer _g_tcp_server_;

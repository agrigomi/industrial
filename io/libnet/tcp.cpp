#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include "private.h"

bool cTCPServer::_init(_u32 port) {
	bool r = false;

	if((m_server_socket = socket(AF_INET, SOCK_STREAM, 0)) > 0) {
		m_port = port;
		m_serveraddr.sin_family = AF_INET;
		m_serveraddr.sin_addr.s_addr = htonl(INADDR_ANY);
		m_serveraddr.sin_port = htons((unsigned short)port);
		if(bind(m_server_socket, (struct sockaddr *)&m_serveraddr, sizeof(m_serveraddr)) >= 0) {
			if(::listen(m_server_socket, 10) >= 0)
				r = true;
			else
				::close(m_server_socket);
		} else
			::close(m_server_socket);

		if(!r)
			m_server_socket = 0;
	}

	return r;
}

void cTCPServer::_close(void) {
	if(m_server_socket) {
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
		} break;
		case OCTL_UNINIT: {
			iRepository *pi_repo = (iRepository *)arg;
			pi_repo->object_release(mpi_heap);
		} break;
	}

	return r;
}

iSocketIO *cTCPServer::listen(bool blocking) {
}



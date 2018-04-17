#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#include "private.h"
#include "iRepository.h"

bool cUDPServer::object_ctl(_u32 cmd, void *arg, ...) {
	bool r = false;

	switch(cmd) {
		case OCTL_INIT:
			m_socket = 0;
			memset(&m_serveraddr, 0, sizeof(m_serveraddr));
			r = true;
			break;
		case OCTL_UNINIT:
			_close();
			r = true;
			break;
	}

	return r;
}

bool cUDPServer::_init(_u32 port) {
	bool r = false;

	m_port = port;
	if((m_socket = socket(AF_INET, SOCK_DGRAM, 0)) != -1) {
		_s32 optv = 1;

		setsockopt(m_socket, SOL_SOCKET, SO_REUSEADDR, (const void *)&optv, sizeof(optv));
		memset(&m_serveraddr, 0, sizeof(m_serveraddr));
		m_serveraddr.sin_family = AF_INET;
		m_serveraddr.sin_addr.s_addr = htonl(INADDR_ANY);
		m_serveraddr.sin_port = htons((unsigned short)m_port);
		if(bind(m_socket, (struct sockaddr *)&m_serveraddr, sizeof(m_serveraddr)) >=0)
			r = true;
		else
			_close();
	}

	return r;
}

void cUDPServer::_close(void) {
	if(m_socket) {
		::close(m_socket);
		m_socket = 0;
	}
}

iSocketIO *cUDPServer::listen(void) {
	iSocketIO *r = 0;

	if(m_socket) {
		cSocketIO *pcsio = (cSocketIO *)_gpi_repo_->object_by_cname(CLASS_NAME_SOCKET_IO, RF_CLONE);
		if(pcsio) {
			struct sockaddr_in sa;
			pcsio->_init(&sa, m_socket, SOCKET_IO_UDP);
			r = pcsio;
		}
	}

	return r;
}

void cUDPServer::close(iSocketIO *p_io) {
	cSocketIO *pcio = dynamic_cast<cSocketIO*>(p_io);
	if(pcio) {
		pcio->_close();
		_gpi_repo_->object_release(p_io);
	}
}

static cUDPServer _g_udp_server_;

void cUDPClient::_init(_str_t ip, _u32 port) {
}

void cUDPClient::_close(void) {
}

bool cUDPClient::object_ctl(_u32 cmd, void *arg, ...) {
	bool r = false;

	switch(cmd) {
		case OCTL_INIT:
			//...
			r = true;
			break;
		case OCTL_UNINIT:
			_close();
			r = true;
			break;
	}

	return r;
}

iSocketIO *cUDPClient::connect(void) {
	iSocketIO *r = 0;
	//...
	return r;
}

static cUDPClient _g_udp_client_;

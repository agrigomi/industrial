#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#include "private.h"

bool cUDPServer::object_ctl(_u32 cmd, void *arg, ...) {
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

void cUDPServer::_init(_u32 port) {
	m_port = port;
}

void cUDPServer::_close(void) {
}

iSocketIO *cUDPServer::listen(bool blocking) {
	iSocketIO *r = 0;
	//...
	return r;
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

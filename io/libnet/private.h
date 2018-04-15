#ifndef __NET_PRIVATE_H__
#define __NET_PRIVATE_H__

#include "iNet.h"

#define CLASS_NAME_SOCKET_IO	"cSocketIO"
#define CLASS_NAME_UDP_SERVER	"cUDPServer"
#define CLASS_NAME_UDP_CLIENT	"cUDPClient"
#define CLASS_NAME_TCP_SERVER	"cTCPServer"
#define CLASS_NAME_TCP_CLIENT	"cTCPClient"

class cSocketIO: public iSocketIO {
public:
	BASE(cSocketIO, CLASS_NAME_SOCKET_IO, RF_CLONE, 1,0,0);
	void _init(_s32 socket);
	void _close(void);
	bool object_ctl(_u32 cmd, void *arg, ...);
	_u32 read(void *data, _u32 size);
	_u32 write(void *data, _u32 size);
	void blocking(bool mode=_blocking_); /* blocking or nonblocking IO */
};

class cUDPServer: public iUDPServer {
public:
	BASE(cUDPServer, CLASS_NAME_UDP_SERVER, RF_CLONE, 1,0,0);
	void _init(_u32 port);
	void _close(void);
	bool object_ctl(_u32 cmd, void *arg, ...);
	iSocketIO *listen(bool blocking=_blocking_);
};

class cTCPServer: public iTCPServer {
public:
	BASE(cTCPServer, CLASS_NAME_TCP_SERVER, RF_CLONE, 1,0,0);
	void _init(_u32 port);
	void _close(void);
	bool object_ctl(_u32 cmd, void *arg, ...);
	iSocketIO *listen(bool blocking=_blocking_);
};

class cUDPClient: public iUDPClient {
public:
	BASE(cUDPClient, CLASS_NAME_UDP_CLIENT, RF_CLONE, 1,0,0);
	void _init(_u32 port);
	void _close(void);
	bool object_ctl(_u32 cmd, void *arg, ...);
	iSocketIO *connect(void);
};

class cTCPClient: public iTCPClient {
public:
	BASE(cTCPClient, CLASS_NAME_TCP_CLIENT, RF_CLONE, 1,0,0);
	void _init(_u32 port);
	void _close(void);
	bool object_ctl(_u32 cmd, void *arg, ...);
	iSocketIO *connect(void);
};

#endif

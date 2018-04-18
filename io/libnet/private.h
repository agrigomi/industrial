#ifndef __NET_PRIVATE_H__
#define __NET_PRIVATE_H__

#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "iNet.h"
#include "iMemory.h"
#include "iRepository.h"

#define CLASS_NAME_SOCKET_IO	"cSocketIO"
#define CLASS_NAME_UDP_SERVER	"cUDPServer"
#define CLASS_NAME_UDP_CLIENT	"cUDPClient"
#define CLASS_NAME_TCP_SERVER	"cTCPServer"
#define CLASS_NAME_TCP_CLIENT	"cTCPClient"

#define SOCKET_IO_UDP	1
#define SOCKET_IO_TCP	2

class cSocketIO: public iSocketIO {
private:
	_s32 m_socket;
	struct sockaddr_in *mp_clientaddr;
	struct sockaddr_in *mp_serveraddr;
	_u8 m_mode;
public:

	BASE(cSocketIO, CLASS_NAME_SOCKET_IO, RF_CLONE, 1,0,0);
	void _init(struct sockaddr_in *p_saddr, // server addr
		struct sockaddr_in *p_caddr, // client addr
		_s32 socket, _u8 mode);
	void _close(void);
	struct sockaddr_in *serveraddr(void);
	struct sockaddr_in *clientaddr(void);
	bool object_ctl(_u32 cmd, void *arg, ...);
	_u32 read(void *data, _u32 size);
	_u32 write(void *data, _u32 size);
	void blocking(bool mode); /* blocking or nonblocking IO */
};

class cTCPServer: public iTCPServer {
private:
	iHeap *mpi_heap;
	_u32 m_port;
	struct sockaddr_in m_serveraddr;
	_s32 m_server_socket;
public:
	BASE(cTCPServer, CLASS_NAME_TCP_SERVER, RF_CLONE, 1,0,0);
	bool _init(_u32 port);
	void _close(void);
	bool object_ctl(_u32 cmd, void *arg, ...);
	iSocketIO *listen(void);
	void blocking(bool mode=true); /* blocking or nonblocking IO */
	void close(iSocketIO *p_io);
};

class cTCPClient: public iTCPClient {
public:
	BASE(cTCPClient, CLASS_NAME_TCP_CLIENT, RF_CLONE, 1,0,0);
	void _init(_str_t ip, _u32 port);
	void _close(void);
	bool object_ctl(_u32 cmd, void *arg, ...);
	iSocketIO *connect(void);
};

#endif

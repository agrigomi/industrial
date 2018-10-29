#ifndef __NET_PRIVATE_H__
#define __NET_PRIVATE_H__

#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <openssl/bio.h>
#include <openssl/ssl.h>
#include <openssl/err.h>
#include "iNet.h"
#include "iMemory.h"
#include "iRepository.h"

#define CLASS_NAME_SOCKET_IO	"cSocketIO"
#define CLASS_NAME_TCP_SERVER	"cTCPServer"

// socket I/O mode
#define SOCKET_IO_UDP	1
#define SOCKET_IO_TCP	2
#define SOCKET_IO_SSL	3

class cSocketIO: public iSocketIO {
private:
	_s32 m_socket;
	struct sockaddr_in *mp_clientaddr;
	struct sockaddr_in *mp_serveraddr;
	_u8 m_mode;
	bool m_alive;
	SSL *mp_cSSL;
public:
	BASE(cSocketIO, CLASS_NAME_SOCKET_IO, RF_CLONE, 1,0,0);
	bool _init(struct sockaddr_in *p_saddr, // server addr
		struct sockaddr_in *p_caddr, // client addr
		_s32 socket, _u8 mode, SSL_CTX *p_ssl_cxt=0);
	void _close(void);
	struct sockaddr_in *serveraddr(void);
	struct sockaddr_in *clientaddr(void);
	bool object_ctl(_u32 cmd, void *arg, ...);
	_u32 read(void *data, _u32 size);
	_u32 write(const void *data, _u32 size);
	void blocking(bool mode); /* blocking or nonblocking IO */
	bool alive(void);
};

class cTCPServer: public iTCPServer {
private:
	iHeap *mpi_heap;
	_u32 m_port;
	struct sockaddr_in m_serveraddr;
	_s32 m_server_socket;
	bool m_use_ssl;
	SSL_CTX *mp_sslcxt;

	void init_ssl(void);
	void destroy_ssl(void);
public:
	BASE(cTCPServer, CLASS_NAME_TCP_SERVER, RF_CLONE, 1,0,0);
	bool _init(_u32 port);
	void _close(void);
	bool object_ctl(_u32 cmd, void *arg, ...);
	bool enable_ssl(bool, _ulong options=0);
	bool ssl_use(_cstr_t str, _u32 type);
	iSocketIO *listen(void);
	void blocking(bool mode=true); /* blocking or nonblocking IO */
	void close(iSocketIO *p_io);
};

#endif

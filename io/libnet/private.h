#ifndef __NET_PRIVATE_H__
#define __NET_PRIVATE_H__

#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include "iNet.h"
#include "iMemory.h"
#include "iRepository.h"
#include "iStr.h"

#define CLASS_NAME_SOCKET_IO		"cSocketIO"
#define CLASS_NAME_TCP_SERVER		"cTCPServer"
#define CLASS_NAME_HTTP_SERVER		"cHttpServer"
#define CLASS_NAME_HTTP_CONNECTION	"cHttpConnection"

// socket I/O mode
#define SOCKET_IO_UDP		1
#define SOCKET_IO_TCP		2
#define SOCKET_IO_SSL_SERVER	3
#define SOCKET_IO_SSL_CLIENT	4

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

class cHttpServer: public iHttpServer {
private:
	cTCPServer	*p_tcps;
	volatile bool	m_is_init;
	volatile bool	m_is_running;
	volatile bool	m_is_stopped;

	friend void http_server_thread(cHttpServer *pobj);

public:
	BASE(cHttpServer, CLASS_NAME_HTTP_SERVER, RF_CLONE | RF_TASK, 1,0,0);
	bool _init(_u32 port);
	void _close(void);
	bool object_ctl(_u32 cmd, void *arg, ...);
	void on_event(_u8 evt, _on_http_event_t *handler);
	bool enable_ssl(bool, _ulong options=0);
	bool ssl_use(_cstr_t str, _u32 type);
};

#define MAX_HTTP_HEADER	4*1024 // 4K

class cHttpConnection: public iHttpConnection {
private:
	cSocketIO	*mp_sio;
	iStr		*mpi_str;
	_u8		m_header[MAX_HTTP_HEADER];
public:
	BASE(cHttpConnection, CLASS_NAME_HTTP_CONNECTION, RF_CLONE, 1,0,0);
	bool object_ctl(_u32 cmd, void *arg, ...);
	bool _init(cSocketIO *p_sio);
	void _close(void);
	bool alive(void);
	// copies value of http header variable into buffer
	bool copy_value(_cstr_t vname, _str_t buffer, _u32 sz_buffer);
	//...
};

#endif

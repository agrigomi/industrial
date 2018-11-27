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
#include "iTaskMaker.h"
#include "iLog.h"
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
	struct sockaddr_in m_clientaddr;
	struct sockaddr_in m_serveraddr;
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
	_u32 peer_ip(void);
	bool peer_ip(_str_t strip, _u32 len);
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

// state bitmap for HttpConnection
#define HTTPC_REQ_PENDING	(1<<0)
#define HTTPC_REQ_HEADER	(1<<1)
#define HTTPC_REQ_BODY		(1<<2)
#define HTTPC_REQ_END		(1<<3)
#define HTTPC_RES_PENDING	(1<<4)
#define HTTPC_RES_HEADER	(1<<5)
#define HTTPC_RES_BODY		(1<<6)
#define HTTPC_RES_END		(1<<7)
#define HTTPC_RES_SENDING	(1<<8)
#define HTTPC_RES_SENT		(1<<9)

class cHttpConnection: public iHttpConnection {
private:
	cSocketIO	*mp_sio;
	iStr		*mpi_str;
	iBufferMap	*mpi_bmap;
	HBUFFER		m_req_buffer;
	_u32		m_req_len;
	_u32		m_req_hdr_len;
	_u32		m_res_len;
	_u16		m_state;
	_u32		m_content_len;
	_u32		m_content_sent;
	_ulong		m_udata;

	_u32 read_request(void);
	bool complete_request(void);
	_cstr_t get_rc_text(_u16 rc);
public:
	BASE(cHttpConnection, CLASS_NAME_HTTP_CONNECTION, RF_CLONE, 1,0,0);
	bool object_ctl(_u32 cmd, void *arg, ...);
	bool _init(cSocketIO *p_sio, iBufferMap *pi_bmap);
	void close(void);
	bool alive(void);
	cSocketIO *get_socket_io(void) {
		return mp_sio;
	}
	_u16 get_status(void) {
		return m_state;
	}
	_u32 peer_ip(void);
	bool peer_ip(_str_t strip, _u32 len);
	void set_udata(_ulong udata) {
		m_udata = udata;
	}
	_ulong get_udata(void) {
		return m_udata;
	}
	_str_t req_header(_u32 *);
	_str_t req_body(_u32 *);
	void process(void);
	_u32 response(_u16 rc, // response code
			_str_t hdr, // response header
			_str_t body, // response body
			// Size of response body.
			// If zero, string length should be taken.
			// If it's greater than body lenght, ON_HTTP_RES_CONTINUE
			//  should be happen
			_u32 sz_body=0
			);
	_u32 response(_str_t body, // remainder part of response body
			_u32 sz_body=0 // size of response body
			);
	_u32 remainder(void);
};

typedef struct {
	cHttpConnection *p_httpc;
	_u8 state;
}_http_connection_t;


class cHttpServer: public iHttpServer {
private:
	cTCPServer		*p_tcps;
	iLog			*mpi_log;
	iBufferMap		*mpi_bmap;
	iTaskMaker		*mpi_tmaker;
	iLlist			*mpi_list;
	volatile bool		m_is_init;
	volatile bool		m_is_running;
	volatile bool		m_is_stopped;
	bool			m_use_ssl;
	volatile _u32		m_num_workers;
	volatile _u32 		m_active_workers;
	_on_http_event_t	*mp_on_connect;
	_on_http_event_t	*mp_on_request;
	_on_http_event_t	*mp_on_continue;
	_on_http_event_t	*mp_on_disconnect;

	friend void *http_worker_thread(void *);

	void http_server_thread(void);
	bool start_worker(void);
	bool stop_worker(void);
	_http_connection_t *add_connection(void);
	_http_connection_t *get_connection(void);
	void free_connection(_http_connection_t *rec);
	void remove_connection(_http_connection_t *rec);
	void clear_column(_u8 col, HMUTEX hlock);
	void remove_all_connections(void);

public:
	BASE(cHttpServer, CLASS_NAME_HTTP_SERVER, RF_CLONE | RF_TASK, 1,0,0);
	bool _init(_u32 port);
	void _close(void);
	bool object_ctl(_u32 cmd, void *arg, ...);
	void on_event(_u8 evt, _on_http_event_t *handler);
	bool enable_ssl(bool, _ulong options=0);
	bool ssl_use(_cstr_t str, _u32 type);
	bool is_running(void) {
		return m_is_running;
	}
};

#endif

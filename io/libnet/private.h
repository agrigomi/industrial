#ifndef __NET_PRIVATE_H__
#define __NET_PRIVATE_H__

#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <time.h>
#include "iNet.h"
#include "iMemory.h"
#include "iRepository.h"
#include "iTaskMaker.h"
#include "iLog.h"
#include "iStr.h"

#define CLASS_NAME_SOCKET_IO		"cSocketIO"
#define CLASS_NAME_TCP_SERVER		"cTCPServer"
#define CLASS_NAME_HTTP_SERVER		"cHttpServer"
#define CLASS_NAME_HTTP_CONNECTION	"cHttpServerConnection"

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
	HOBJECT m_hsio;

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

#define HTTPC_MAX_UDATA_INDEX	8

class cHttpServerConnection: public iHttpServerConnection {
private:
	cSocketIO	*mp_sio;
	iStr		*mpi_str;
	iBufferMap	*mpi_bmap;
	iMap		*mpi_map;  // request variables container
	HBUFFER		m_ibuffer; // input buffer
	HBUFFER		m_oheader; // output header
	HBUFFER		m_obuffer; // output buffer
	_u32 		m_ibuffer_offset;
	_u32		m_oheader_offset;
	_u32		m_obuffer_offset;
	_u32		m_oheader_sent;
	_u32		m_obuffer_sent;
	_u32		m_header_len;
	_u32		m_res_content_len;
	_u32		m_req_content_len;
	_u32		m_req_content_rcv;
	_u32		m_content_sent;
	_u16		m_error_code;
	_u16		m_response_code;
	_u16		m_state;
	time_t		m_stime;
	_u32		m_timeout;
	_ulong		m_udata[HTTPC_MAX_UDATA_INDEX];
	_char_t		m_res_protocol[16];

	_cstr_t get_rc_text(_u16 rc);
	bool complete_req_header(void);
	bool add_req_variable(_cstr_t name, _cstr_t value, _u32 sz_value=0);
	bool parse_req_header(void);
	_u32 parse_request_line(_str_t req, _u32 sz_max);
	_u32 parse_var_line(_str_t var, _u32 sz_max);
	_u32 parse_url(_str_t url, _u32 sz_max);
	_u32 receive(void);
	void clear_ibuffer(void);
	_u32 send_header(void);
	_u32 send_content(void);
	_u32 receive_content(void);
	void clean_members(void);
	void release_buffers(void);

public:
	BASE(cHttpServerConnection, CLASS_NAME_HTTP_CONNECTION, RF_CLONE, 1,0,0);
	bool object_ctl(_u32 cmd, void *arg, ...);
	bool _init(cSocketIO *p_sio, iBufferMap *pi_bmap, _u32 timeout);
	void close(void);
	bool alive(void);
	_u8 process(void);
	cSocketIO *get_socket_io(void) {
		return mp_sio;
	}
	_u32 peer_ip(void);
	bool peer_ip(_str_t strip, _u32 len);
	void set_udata(_ulong udata, _u8 index=0) {
		if(index < HTTPC_MAX_UDATA_INDEX)
			m_udata[index] = udata;
	}
	_ulong get_udata(_u8 index=0) {
		_ulong r = 0;

		if(index < HTTPC_MAX_UDATA_INDEX)
			r = m_udata[index];
		return r;
	}
	// get request method
	_u8 req_method(void);
	// retuen request HEADER
	_cstr_t req_header(void);
	// retuen request URI
	_cstr_t req_uri(void);
	// retuen request URL
	_cstr_t req_url(void);
	// return request URN
	_cstr_t req_urn(void);
	// get request variable
	_cstr_t req_var(_cstr_t name);
	// get request data
	_u8 *req_data(_u32 *size);
	// get requested protocol
	_cstr_t req_protocol(void);
	// set variable in response header
	bool res_var(_cstr_t name, _cstr_t value);
	// get error code
	_u16 error_code(void) {
		return m_error_code;
	}
	// set response protocol as string like 'HTTP/1.1'
	 void res_protocol(_cstr_t protocol);
	// set response code
	void res_code(_u16 httprc) {
		m_response_code = httprc;
	}
	// set Content-Length variable
	bool res_content_len(_u32 content_len);
	// return content len of response
	_u32 res_content_len(void) {
		return m_res_content_len;
	}
	// return nimber of sent bytes for response content
	_u32 res_content_sent(void) {
		return m_content_sent;
	}
	// set last modify time in response header
	void res_mtime(time_t mtime);
	// returns the content length of  request
	_u32 req_content_len(void) {
		return m_req_content_len;
	}
	// return remainder pard of response data in bytes (ContentLength - Sent)
	_u32 res_remainder(void);
	// write response
	_u32 res_write(_u8 *data, _u32 size);
	_u32 res_write(_cstr_t str);
};

typedef struct {
	cHttpServerConnection *p_httpc;
	_u8 state;
}_http_connection_t;

typedef struct {
	_on_http_event_t	*pf_handler;
	void			*udata;
}_http_event_t;

#define HTTP_MAX_EVENTS	10

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
	_http_event_t		m_event[HTTP_MAX_EVENTS];
	HOBJECT			m_hconnection;
	_u32			m_max_workers;
	_u32			m_max_connections;
	_u32			m_connection_timeout;
	_u32			m_num_connections;

	friend void *_http_worker_thread(void *);
	friend void *_http_server_thread(void *);

	void http_server_thread(void);
	bool start_worker(void);
	bool stop_worker(void);
	_http_connection_t *add_connection(void);
	_http_connection_t *get_connection(void);
	void free_connection(_http_connection_t *rec);
	void remove_connection(_http_connection_t *rec);
	void clear_column(_u8 col, HMUTEX hlock);
	void remove_all_connections(void);
	bool call_event_handler(_u8 evt, iHttpServerConnection *pi_httpc);
public:
	BASE(cHttpServer, CLASS_NAME_HTTP_SERVER, RF_CLONE, 1,0,0);
	bool _init(_u32 port,
			_u32 buffer_size=8192,
			_u32 max_workers=32,
			_u32 max_connections=500,
			_u32 connection_timeout=10);
	void _close(void);
	bool object_ctl(_u32 cmd, void *arg, ...);
	void on_event(_u8 evt, _on_http_event_t *handler, void *udata=NULL);
	bool enable_ssl(bool, _ulong options=0);
	bool ssl_use(_cstr_t str, _u32 type);
	bool is_running(void) {
		return m_is_running;
	}
};

#endif

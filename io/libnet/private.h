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

#define CLASS_NAME_SOCKET_IO			"cSocketIO"
#define CLASS_NAME_TCP_SERVER			"cTCPServer"
#define CLASS_NAME_HTTP_SERVER			"cHttpServer"
#define CLASS_NAME_HTTP_SERVER_CONNECTION	"cHttpServerConnection"
#define CLASS_NAME_HTTP_CLIENT_CONNECTION	"cHttpClientConnection"
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
	_s32 socket(void) {
		return m_socket;
	}
};

class cTCPServer: public iTCPServer {
private:
	iHeap *mpi_heap;
	_u32 m_port;
	struct sockaddr_in m_serveraddr;
	_s32 m_server_socket;
	SSL_CTX *mp_sslcxt;
	HOBJECT m_hsio;

	void init_ssl(void);
	void destroy_ssl(void);
public:
	BASE(cTCPServer, CLASS_NAME_TCP_SERVER, RF_CLONE, 1,0,0);
	bool _init(_u32 port, SSL_CTX *ssl_context=NULL);
	void _close(void);
	bool object_ctl(_u32 cmd, void *arg, ...);
	iSocketIO *listen(void);
	void blocking(bool mode=true); /* blocking or nonblocking IO */
	void close(iSocketIO *p_io);
};

#define HTTPC_MAX_UDATA_INDEX	8

class cHttpServerConnection: public iHttpServerConnection {
private:
	cSocketIO	*mp_sio;
	iBufferMap	*mpi_bmap;
	iMap		*mpi_req_map;  // request variables container
	iMap		*mpi_res_map; // response header
	iLlist		*mpi_cookie_list;
	HBUFFER		m_ibuffer; // input buffer
	HBUFFER		m_oheader; // output header
	HBUFFER		m_obuffer; // output buffer
	_u32 		m_ibuffer_offset;
	_u32		m_oheader_offset;
	_u32		m_obuffer_offset;
	_u32		m_oheader_sent;
	_u32		m_obuffer_sent;
	_u32		m_header_len;
	_ulong		m_res_content_len;
	_u32		m_req_content_len;
	_u32		m_req_content_rcv;
	_ulong		m_content_sent;
	_u16		m_error_code;
	_u16		m_response_code;
	_u16		m_state;
	time_t		m_stime;
	_u32		m_timeout;
	_ulong		m_udata[HTTPC_MAX_UDATA_INDEX];
	_char_t		m_res_protocol[16];
	bool 		m_req_data;
	_cstr_t		m_content_type;
	void		*mp_doc;
	bool		m_res_hdr_prepared;

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
	void prepare_res_header(void);

public:
	BASE(cHttpServerConnection, CLASS_NAME_HTTP_SERVER_CONNECTION, RF_CLONE, 1,0,0);
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
	// parse request content
	bool req_parse_content(void);
	// set variable in response header
	bool res_var(_cstr_t name, _cstr_t value);
	// get error code
	_u16 error_code(void) {
		return m_error_code;
	}
	// get response text
	_cstr_t res_text(_u16 rc) {
		return get_rc_text(rc);
	}
	// set response protocol as string like 'HTTP/1.1'
	void res_protocol(_cstr_t protocol);
	// set response code
	void res_code(_u16 httprc) {
		m_error_code = m_response_code = httprc;
	}
	// set Content-Length variable
	bool res_content_len(_ulong content_len);
	// return content len of response
	_ulong res_content_len(void) {
		return m_res_content_len;
	}
	// return nimber of sent bytes for response content
	_ulong res_content_sent(void) {
		return m_content_sent;
	}

	void res_content_type(_cstr_t ctype) {
		m_content_type = ctype;
	}

	// Set document content
	void res_content(void *p_doc, _ulong sz_doc) {
		mp_doc = p_doc;
		m_res_content_len = sz_doc;
	}

	// set last modify time in response header
	void res_mtime(time_t mtime);
	// returns the content length of  request
	_u32 req_content_len(void) {
		return m_req_content_len;
	}
	// Set response cookie
	void res_cookie(_cstr_t name,
			_cstr_t value,
			_u8 flags=0,
			_cstr_t expires=NULL,
			_cstr_t max_age=NULL,
			_cstr_t path=NULL,
			_cstr_t domain=NULL);
	// return remainder pard of response data in bytes (ContentLength - Sent)
	_ulong res_remainder(void);
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
	_u32			m_port;

	friend void *_http_worker_thread(_u8 sig, void *);
	friend void *_http_server_thread(_u8 sig, void *);

	void http_server_thread(void);
	bool start_worker(void);
	bool stop_worker(void);
	_http_connection_t *add_connection(void);
	_http_connection_t *get_connection(void);
	_http_connection_t *alloc_connection(HMUTEX hlock);
	void pending_connection(_http_connection_t *rec);
	void release_connection(_http_connection_t *rec);
	void clear_column(_u8 col, HMUTEX hlock);
	void remove_all_connections(void);
	bool call_event_handler(_u8 evt, iHttpServerConnection *pi_httpc);
public:
	BASE(cHttpServer, CLASS_NAME_HTTP_SERVER, RF_CLONE, 1,0,0);
	bool _init(_u32 port,
			_u32 buffer_size=HTTP_BUFFER_SIZE,
			_u32 max_workers=HTTP_MAX_WORKERS,
			_u32 max_connections=HTTP_MAX_CONNECTIONS,
			_u32 connection_timeout=HTTP_CONNECTION_TIMEOUT,
			SSL_CTX *ssl_context=NULL);
	void _close(void);
	bool object_ctl(_u32 cmd, void *arg, ...);
	void on_event(_u8 evt, _on_http_event_t *handler, void *udata=NULL);
	bool is_running(void) {
		return m_is_running;
	}
};

class cHttpClientConnection: public iHttpClientConnection {
private:
	iSocketIO	*mpi_sio;
	iMap		*mpi_map;
	iHeap		*mpi_heap;
	iStr		*mpi_str;
	_u32		m_buffer_size;
	void		**mpp_buffer_array;
	_char_t		*mp_bheader;
	_u32		m_sz_barray;
	_u8		m_req_method;
	_u32		m_content_len;
	_u32		m_header_len;
	_u16		m_res_code;

	void *alloc_buffer(void);
	void *calc_buffer(_u32 *sz);
	_u32 write_buffer(void *data, _u32 size);
	bool prepare_req_header(void);
	bool parse_response_header(void);
	_u32 receive_buffer(void *buffer, time_t now, _u32 timeout_s);
	bool add_var(_cstr_t vname, _u32 sz_vname, _cstr_t vvalue, _u32 sz_vvalue);
	_u32 parse_response_line(void);
	_u32 parse_variable_line(_cstr_t var, _u32 sz_max);
public:
	BASE(cHttpClientConnection, CLASS_NAME_HTTP_CLIENT_CONNECTION, RF_CLONE, 1,0,0);
	bool _init(iSocketIO *pi_sio, _u32 buffer_size);
	bool object_ctl(_u32 cmd, void *arg, ...);
	// reset object members
	void reset(void);
	// check connection
	bool alive(void);
	// set request method
	void req_method(_u8 method) {
		m_req_method = method;
	}
	// set protocol
	void req_protocol(_cstr_t protocol);
	// set request URL
	void req_url(_cstr_t url);
	// set request variable
	bool req_var(_cstr_t name, _cstr_t value);
	// Write request content
	_u32 req_write(_u8 *data, _u32 size);
	_u32 req_write(_cstr_t str);
	_u32 _req_write(_cstr_t fmt, ...);
	// send request with timeout in seconds
	bool send(_u32 timeout_s, _on_http_response_t *p_cb_resp=NULL, void *udata=NULL);
	// get response code
	_u16 res_code(void) {
		return m_res_code;
	}
	// get value from response variable
	_cstr_t res_var(_cstr_t name);
	// get response content len
	_u32 res_content_len(void) {
		return m_content_len;
	}
	// get response content
	void res_content(_on_http_response_t *p_cb_resp, void *udata=NULL);
};

#endif

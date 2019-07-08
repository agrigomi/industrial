#include "iMemory.h"
#include "iGatn.h"
#include "iLog.h"

struct request: public _request_t{
	iHttpServerConnection *mpi_httpc;
	_server_t *mpi_server;

	iHttpServerConnection *connection(void) {
		return mpi_httpc;
	}

	_server_t *server(void) {
		return mpi_server;
	}

	_u8 method(void);
	_cstr_t header(void);
	_cstr_t utl(void);
	_cstr_t urn(void);
	_cstr_t var(_cstr_t name);
	_u32 content_len(void);
	void *data(_u32 *size);
	void destroy(void);
};

struct response: public _response_t {
	iHttpServerConnection *mpi_httpc;
	_server_t *mpi_server;
	iHeap *mpi_heap;
	iBufferMap *mpi_bmap;
	HBUFFER	*mp_hbarray;
	_u32 m_hbcount; // array capacity
	_u32 m_buffers; // allocated buffers
	_u32 m_content_len;
	_cstr_t m_doc_root;
	iFileCache *mpi_fcache;
	iFS *mpi_fs;

	iHttpServerConnection *connection(void) {
		return mpi_httpc;
	}

	_server_t *server(void) {
		return mpi_server;
	}

	bool alloc_buffer(void);
	void clear(void);
	_u32 capacity(void);
	_u32 remainder(void);
	bool resize_array(void);
	void var(_cstr_t name, _cstr_t value);
	void _var(_cstr_t name, _cstr_t fmt, ...);
	_u32 write(void *data, _u32 size);
	_u32 write(_cstr_t str);
	_u32 _write(_cstr_t fmt, ...);
	_u32 end(_u16 response_code, void *data, _u32 size);
	_u32 end(_u16 response_code, _cstr_t str);
	_u32 _end(_u16 response_code, _cstr_t fmt, ...);
	void destroy(void);
	void process_content(void);
	void redirect(_cstr_t uri);
	bool render(_cstr_t fname,
		_u8 flags=RNDR_DONE|RNDR_CACHE|RNDR_RESOLVE_MT|RNDR_SET_MTIME);
};

#define MAX_SERVER_NAME		32
#define SERVER_BUFFER_SIZE	16384
#define HTTP_MAX_EVENTS		10
#define MAX_DOC_ROOT_PATH	512
#define MAX_CACHE_PATH		512
#define MAX_HOSTNAME		256

typedef struct {
	_on_http_event_t	*pcb;
	void			*udata;
}_event_data_t;

void init_mime_type_resolver(void);
void uninit_mime_type_resolver(void);
_cstr_t resolve_mime_type(_cstr_t fname);

typedef struct {
	_char_t		host[MAX_HOSTNAME];	// host name
	_char_t 	root[MAX_DOC_ROOT_PATH];// document root
	_char_t		cache_path[MAX_CACHE_PATH];
	iFileCache	*pi_fcache;		// file cache
	iMap		*pi_nocache_map;	// non cacheable areas in document root
	iMap		*pi_route_map;
	_event_data_t	event[HTTP_MAX_EVENTS];
}_vhost_t;

struct server: public _server_t {
	_char_t 	m_name[MAX_SERVER_NAME];
	_u32 		m_port;
	iHttpServer 	*mpi_server;
	_vhost_t	host;
	iMap		*mpi_vhost_map; // virtual hosts map
	iNet		*mpi_net; // networking
	iFS		*mpi_fs; // FS support
	iLog		*mpi_log; // system log
	iHeap		*mpi_heap;
	bool		m_autorestore;
	iBufferMap	*mpi_bmap;
	_u32		m_buffer_size;
	_u32		m_max_workers;
	_u32		m_max_connections;
	_u32		m_connection_timeout;
	SSL_CTX		*m_ssl_context;

	bool is_running(void) {
		return (mpi_server) ? true : false;
	}
	bool start(void);
	void stop(void);
	_cstr_t name(void) {
		return m_name;
	}
	_u32 port(void) {
		return m_port;
	}
	bool create_connection(iHttpServerConnection *p_httpc);
	void destroy_connection(iHttpServerConnection *p_httpc);
	_cstr_t resolve_content_type(_cstr_t doc_name);
	void set_handlers(void);
	void call_handler(_u8 evt, iHttpServerConnection *p_httpc);
	void call_route_handler(_u8 evt, iHttpServerConnection *p_httpc);
	void update_response(iHttpServerConnection *p_httpc);
	void on_route(_u8 method, _cstr_t path, _on_route_event_t *pcb, void *udata=NULL);
	void on_event(_u8 evt, _on_http_event_t *pcb, void *udata=NULL);
	void remove_route(_u8 method, _cstr_t path);
	_request_t *get_request(iHttpServerConnection *pi_httpc);
	_response_t *get_response(iHttpServerConnection *pi_httpc);
	iFileCache *get_file_cache(void) {
		return host.pi_fcache;
	}
	void enum_route(void (*)(_cstr_t path, _on_route_event_t *pcb, void *udata), void *udata=NULL);
};

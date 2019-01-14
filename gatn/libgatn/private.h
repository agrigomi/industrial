#include "iMemory.h"
#include "iGatn.h"
#include "iLog.h"

struct request: public _request_t{
	iHttpConnection *mpi_httpc;

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
	iHttpConnection *mpi_httpc;
	iHeap *mpi_heap;
	iBufferMap *mpi_bmap;
	HBUFFER	*mp_hbarray;
	_u32 m_hbcount; // array capacity
	_u32 m_buffers; // allocated buffers
	_u32 m_content_len;

	bool alloc_buffer(void);
	_u32 capacity(void);
	_u32 remainder(void);
	bool resize_array(void);
	void var(_cstr_t name, _cstr_t value);
	_u32 write(void *data, _u32 size);
	_u32 end(_u16 response_code, void *data, _u32 size);
	void destroy(void);
	void process_content(void);
	void redirect(_cstr_t uri);
};

#define MAX_SERVER_NAME		32
#define SERVER_BUFFER_SIZE	16384
#define GATN_BUFFER_SIZE	32*1024
#define HTTP_MAX_EVENTS		10
#define MAX_DOC_ROOT_PATH	512
#define MAX_CACHE_PATH		512

typedef struct {
	_on_http_event_t	*pcb;
	void			*udata;
}_event_data_t;

_cstr_t resolve_mime_type(_cstr_t fname);

struct server: public _server_t {
	_char_t 	m_name[MAX_SERVER_NAME];
	_u32 		m_port;
	iHttpServer 	*mpi_server;
	iMap		*mpi_map; // route map
	iNet		*mpi_net; // networking
	iFS		*mpi_fs; // FS support
	iLog		*mpi_log; // system log
	iHeap		*mpi_heap;
	bool		m_autorestore;
	_event_data_t	m_event[HTTP_MAX_EVENTS];
	iBufferMap	*mpi_bmap;
	iFileCache	*mpi_fcache;
	_char_t		m_doc_root[MAX_DOC_ROOT_PATH];
	_char_t		m_cache_path[MAX_CACHE_PATH];

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
	bool create_connection(iHttpConnection *p_httpc);
	_cstr_t resolve_content_type(_cstr_t doc_name);
	void set_handlers(void);
	void call_handler(_u8 evt, iHttpConnection *p_httpc);
	void call_route_handler(_u8 evt, iHttpConnection *p_httpc);
	void update_response(iHttpConnection *p_httpc);
	void on_route(_u8 method, _cstr_t path, _on_route_event_t *pcb, void *udata);
	void on_event(_u8 evt, _on_http_event_t *pcb, void *udata);
	void remove_route(_u8 method, _cstr_t path);
	_request_t *get_request(iHttpConnection *pi_httpc);
	_response_t *get_response(iHttpConnection *pi_httpc);
};

#include "iMemory.h"
#include "iGatn.h"
#include "iLog.h"

struct request: public _request_t{
	iHttpConnection *mpi_httpc;

	_cstr_t header(void);
	_cstr_t utl(void);
	_cstr_t urn(void);
	_cstr_t var(_cstr_t name);
	_u32 content_len(void);
	void *data(_u32 *size);
};

struct response: public _response_t {
	iHttpConnection *mpi_httpc;

	void var(_cstr_t name, _cstr_t value);
	_u32 write(void *data, _u32 size);
	_u32 end(_u16 response_code, void *data, _u32 size);
};

#define MAX_SERVER_NAME		32
#define SERVER_BUFFER_SIZE	16384
#define HTTP_MAX_EVENTS		10

typedef struct {
	_on_http_event_t	*pcb;
	void			*udata;
}_event_data_t;

struct server: public _server_t {
	_char_t 	m_name[MAX_SERVER_NAME];
	_u32 		m_port;
	iHttpServer 	*mpi_server;
	iMap		*mpi_map; // route map
	iNet		*mpi_net; // networking
	iFS		*mpi_fs; // FS support
	iLog		*mpi_log; // system log
	bool		m_autorestore;
	_event_data_t	m_event[HTTP_MAX_EVENTS];

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
	void set_handlers(void);
	void call_handler(_u8 evt, iHttpConnection *p_httpc);
	void on_route(_u8 method, _cstr_t path, _on_route_event_t *pcb, void *udata);
	void on_event(_u8 evt, _on_http_event_t *pcb, void *udata);
	void remove_route(_u8 method, _cstr_t path);
};

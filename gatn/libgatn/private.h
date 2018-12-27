#include "iMemory.h"
#include "iGatn.h"

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

	void code(_u16 rc);
	void var(_cstr_t name, _cstr_t value);
	_u32 write(void *data, _u32 size);
	_u32 end(void *data, _u32 size);
};

struct server: public _server_t {
	_cstr_t m_name;
	_u32 m_port;
	iHttpServer *mpi_server;

	bool start(void);
	void stop(void);
	_cstr_t name(void) {
		return m_name;
	}
	_u32 port(void) {
		return m_port;
	}
	void on_route(_u8 method, _cstr_t path, _on_route_event_t *pcb, void *udata);
	void on_event(_u8 evt, _on_http_event_t *pcb, void *udata);
};

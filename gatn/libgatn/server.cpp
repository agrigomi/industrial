#include "iGatn.h"
#include "iRepository.h"
#include "private.h"

#define MAX_ROUTE_PATH	256

typedef struct {
	_u8	method;
	_char_t	path[MAX_ROUTE_PATH];
}_route_key_t;

typedef struct {
	_on_route_event_t	*pcb;
	void			*udata;
}_route_data_t;

void server::set_handlers(void) {
	mpi_server->on_event(HTTP_ON_OPEN, [](iHttpConnection *p_httpc, void *udata) {
		server *p_srv = (server *)udata;

		p_srv->call_handler(HTTP_ON_OPEN, p_httpc);
	}, this);

	mpi_server->on_event(HTTP_ON_REQUEST, [](iHttpConnection *p_httpc, void *udata) {
		server *p_srv = (server *)udata;

		p_srv->call_handler(HTTP_ON_REQUEST, p_httpc);
		p_srv->call_route_handler(HTTP_ON_REQUEST, p_httpc);
	}, this);

	mpi_server->on_event(HTTP_ON_REQUEST_DATA, [](iHttpConnection *p_httpc, void *udata) {
		server *p_srv = (server *)udata;

		p_srv->call_handler(HTTP_ON_REQUEST_DATA, p_httpc);
		p_srv->call_route_handler(HTTP_ON_REQUEST_DATA, p_httpc);
	}, this);

	mpi_server->on_event(HTTP_ON_RESPONSE_DATA, [](iHttpConnection *p_httpc, void *udata) {
		server *p_srv = (server *)udata;

		p_srv->call_handler(HTTP_ON_RESPONSE_DATA, p_httpc);
		p_srv->update_response(p_httpc);
	}, this);

	mpi_server->on_event(HTTP_ON_ERROR, [](iHttpConnection *p_httpc, void *udata) {
		server *p_srv = (server *)udata;

		p_srv->call_handler(HTTP_ON_ERROR, p_httpc);
		p_srv->call_route_handler(HTTP_ON_ERROR, p_httpc);
	}, this);

	mpi_server->on_event(HTTP_ON_CLOSE, [](iHttpConnection *p_httpc, void *udata) {
		server *p_srv = (server *)udata;

		p_srv->call_handler(HTTP_ON_CLOSE, p_httpc);
	}, this);
}

bool server::start(void) {
	bool r = false;

	if(!mpi_server) {
		if(mpi_map && mpi_net && mpi_fs) {
			if((mpi_server = mpi_net->create_http_server(m_port, SERVER_BUFFER_SIZE))) {
				set_handlers();
				r = true;
			}
		}
	} else
		r = true;

	return r;
}

void server::stop(void) {
	if(mpi_server) {
		_gpi_repo_->object_release(mpi_server);
		mpi_server = NULL;
	}
}

void server::call_handler(_u8 evt, iHttpConnection *p_httpc) {
	if(evt < HTTP_MAX_EVENTS) {
		if(m_event[evt].pcb)
			m_event[evt].pcb(p_httpc, m_event[evt].udata);
	}
}

void server::call_route_handler(_u8 evt, iHttpConnection *p_httpc) {
	_route_key_t key;
	_u32 sz=0;
	_cstr_t url = p_httpc->req_url();

	// response header
	p_httpc->res_var("Server", m_name);

	memset(&key, 0, sizeof(_route_key_t));
	key.method = p_httpc->req_method();
	strncpy(key.path, url, sizeof(key.path));

	_route_data_t *prd = (_route_data_t *)mpi_map->get(&key, sizeof(_route_key_t), &sz);
	if(prd) {
		// route found
		//...
	} else {
		// route not found
		//...
		p_httpc->res_code(HTTPRC_NOT_FOUND);
	}
}

void server::update_response(iHttpConnection *p_httpc) {
	// write response remainder
	//...
}

void server::on_route(_u8 method, _cstr_t path, _on_route_event_t *pcb, void *udata) {
	_route_key_t key;
	_route_data_t data = {pcb, udata};

	memset(&key, 0, sizeof(_route_key_t));
	key.method = method;
	strncpy(key.path, path, MAX_ROUTE_PATH-1);
	if(mpi_map)
		mpi_map->add(&key, sizeof(_route_key_t), &data, sizeof(_route_data_t));
}

void server::on_event(_u8 evt, _on_http_event_t *pcb, void *udata) {
	if(evt < HTTP_MAX_EVENTS) {
		m_event[evt].pcb = pcb;
		m_event[evt].udata = udata;
	}
}

void server::remove_route(_u8 method, _cstr_t path) {
	_route_key_t key;

	memset(&key, 0, sizeof(_route_key_t));
	key.method = method;
	strncpy(key.path, path, MAX_ROUTE_PATH-1);
	if(mpi_map)
		mpi_map->del(&key, sizeof(_route_key_t));
}


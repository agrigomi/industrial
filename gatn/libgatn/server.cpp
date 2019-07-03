#include <string.h>
#include "iGatn.h"
#include "iRepository.h"
#include "private.h"

#define MAX_ROUTE_PATH	256

#define IDX_FCACHE	6
#define IDX_CONNECTION	7

typedef struct {
	_u8	method;
	_char_t	path[MAX_ROUTE_PATH];

	_u32 size(void) {
		return sizeof(_u8) + strlen(path);
	}
}_route_key_t;

typedef struct {
	_on_route_event_t	*pcb;
	void			*udata;
	_char_t			path[MAX_ROUTE_PATH];

	_u32 size(void) {
		return (sizeof(_on_route_event_t *) + sizeof(void *) + strlen(path));
	}
}_route_data_t;

typedef struct {
	request		req;
	response	res;
}_connection_t;

bool server::create_connection(iHttpServerConnection *p_httpc) {
	bool r = false;
	_connection_t tmp;
	_connection_t *p_cnt = (_connection_t *)mpi_heap->alloc(sizeof(_connection_t));

	if(p_cnt) {
		memcpy((void *)p_cnt, (void *)&tmp, sizeof(_connection_t));
		p_cnt->req.mpi_httpc = p_cnt->res.mpi_httpc = p_httpc;
		p_cnt->res.mpi_heap = mpi_heap;
		p_cnt->res.mpi_bmap = mpi_bmap;
		p_cnt->res.mp_hbarray = NULL;
		p_cnt->res.m_hbcount = 0;
		p_cnt->res.m_content_len = 0;
		p_cnt->res.m_buffers = 0;
		p_cnt->res.m_doc_root = m_doc_root;
		p_cnt->res.mpi_fcache = mpi_fcache;
		p_cnt->res.mpi_fs = mpi_fs;
		p_httpc->set_udata((_ulong)p_cnt, IDX_CONNECTION);
		r = true;
	}

	return r;
}

void server::destroy_connection(iHttpServerConnection *p_httpc) {
	_connection_t *pc = (_connection_t *)p_httpc->get_udata(IDX_CONNECTION);

	if(pc) {
		// release connection memory
		pc->req.destroy();
		pc->res.destroy();
		mpi_heap->free(pc, sizeof(_connection_t));
		p_httpc->set_udata((_ulong)NULL, IDX_CONNECTION);
	}
}

void server::set_handlers(void) {
	mpi_server->on_event(HTTP_ON_OPEN, [](iHttpServerConnection *p_httpc, void *udata) {
		server *p_srv = (server *)udata;

		p_srv->create_connection(p_httpc);
		p_srv->call_handler(HTTP_ON_OPEN, p_httpc);
	}, this);

	mpi_server->on_event(HTTP_ON_REQUEST, [](iHttpServerConnection *p_httpc, void *udata) {
		server *p_srv = (server *)udata;

		/************************************
		 Check for unclosed file cache and connections
		 because in cases of reuse connection (connection: keep-alive),
		 HTTP_ON_CLOSE will not happened
		*/
		HFCACHE old_fc = (HFCACHE)p_httpc->get_udata(IDX_FCACHE);
		if(old_fc) {
			p_srv->mpi_fcache->close(old_fc);
			p_httpc->set_udata(0, IDX_FCACHE);
		}

		_connection_t *pc = (_connection_t *)p_httpc->get_udata(IDX_CONNECTION);
		if(pc)
			pc->res.clear();
		/**********************************/

		p_srv->call_handler(HTTP_ON_REQUEST, p_httpc);
		p_srv->call_route_handler(HTTP_ON_REQUEST, p_httpc);
	}, this);

	mpi_server->on_event(HTTP_ON_REQUEST_DATA, [](iHttpServerConnection *p_httpc, void *udata) {
		server *p_srv = (server *)udata;

		p_srv->call_handler(HTTP_ON_REQUEST_DATA, p_httpc);
		p_srv->call_route_handler(HTTP_ON_REQUEST_DATA, p_httpc);
	}, this);

	mpi_server->on_event(HTTP_ON_RESPONSE_DATA, [](iHttpServerConnection *p_httpc, void *udata) {
		server *p_srv = (server *)udata;

		p_srv->call_handler(HTTP_ON_RESPONSE_DATA, p_httpc);
		p_srv->update_response(p_httpc);
	}, this);

	mpi_server->on_event(HTTP_ON_ERROR, [](iHttpServerConnection *p_httpc, void *udata) {
		server *p_srv = (server *)udata;

		p_srv->call_handler(HTTP_ON_ERROR, p_httpc);
		p_srv->call_route_handler(HTTP_ON_ERROR, p_httpc);
	}, this);

	mpi_server->on_event(HTTP_ON_CLOSE, [](iHttpServerConnection *p_httpc, void *udata) {
		server *p_srv = (server *)udata;
		HFCACHE fc = (HFCACHE)p_httpc->get_udata(IDX_FCACHE);

		p_srv->call_handler(HTTP_ON_CLOSE, p_httpc);
		p_srv->call_route_handler(HTTP_ON_CLOSE, p_httpc);

		if(fc)
			p_srv->mpi_fcache->close(fc);

		p_srv->destroy_connection(p_httpc);
	}, this);
}

bool server::start(void) {
	bool r = false;

	if(mpi_map && mpi_net && mpi_fs) {
		if(!mpi_fcache) { // create file cache
			if((mpi_fcache = dynamic_cast<iFileCache *>(_gpi_repo_->object_by_iname(I_FILE_CACHE, RF_CLONE|RF_NONOTIFY))))
				mpi_fcache->init(m_cache_path, m_name);
		}
		if(!mpi_server)
			mpi_server = mpi_net->create_http_server(m_port, m_buffer_size, m_max_workers,
								m_max_connections, m_connection_timeout,
								m_ssl_context);

		if(mpi_fcache && mpi_server) {
			set_handlers();
			r = true;
		}
	}

	return r;
}

void server::stop(void) {
	if(mpi_server) {
		_gpi_repo_->object_release(mpi_server, false);
		mpi_server = NULL;
	}
	if(mpi_fcache) {
		_gpi_repo_->object_release(mpi_fcache, false);
		mpi_fcache = NULL;
	}
}

void server::call_handler(_u8 evt, iHttpServerConnection *p_httpc) {
	if(evt < HTTP_MAX_EVENTS) {
		if(m_event[evt].pcb)
			m_event[evt].pcb(p_httpc, m_event[evt].udata);
	}
}

_cstr_t server::resolve_content_type(_cstr_t doc_name) {
	_cstr_t r = resolve_mime_type(doc_name);
	return (r) ? r : "";
}

void server::call_route_handler(_u8 evt, iHttpServerConnection *p_httpc) {
	_route_key_t key;
	_u32 sz=0;
	_cstr_t url = p_httpc->req_url();

	if(url) {
		memset(&key, 0, sizeof(_route_key_t));
		key.method = p_httpc->req_method();
		strncpy(key.path, url, sizeof(key.path)-1);

		_route_data_t *prd = (_route_data_t *)mpi_map->get(&key, key.size(), &sz);

		if(prd) {
			// route found
			_connection_t *pc = (_connection_t *)p_httpc->get_udata(IDX_CONNECTION);
			if(pc) // call route handler
				prd->pcb(evt, &pc->req, &pc->res, prd->udata);
		} else if(evt == HTTP_ON_REQUEST) {
			// route not found
			// try to resolve file name
			_char_t doc[MAX_DOC_ROOT_PATH * 2]="";

			if((strlen(url) + strlen(m_doc_root) < sizeof(doc)-1)) {
				snprintf(doc, sizeof(doc), "%s%s",
					m_doc_root,
					(strcmp(url, "/") == 0) ? "/index.html" : url);
				HFCACHE fc = mpi_fcache->open(doc);
				if(fc) {
					if(key.method == HTTP_METHOD_GET) {
						_ulong doc_sz = 0;

						_u8 *ptr = (_u8 *)mpi_fcache->ptr(fc, &doc_sz);
						if(ptr) {
							p_httpc->res_content_len(doc_sz);
							p_httpc->res_code(HTTPRC_OK);
							// response header
							p_httpc->res_var("Server", m_name);
							p_httpc->res_var("Content-Type", resolve_content_type(doc));
							p_httpc->res_mtime(mpi_fcache->mtime(fc));
							p_httpc->res_write(ptr, doc_sz);
							p_httpc->set_udata((_ulong)fc, IDX_FCACHE);
						} else {
							p_httpc->res_code(HTTPRC_INTERNAL_SERVER_ERROR);
							mpi_fcache->close(fc);
						}
					} else if(key.method == HTTP_METHOD_HEAD) {
						p_httpc->res_mtime(mpi_fcache->mtime(fc));
						p_httpc->res_code(HTTPRC_OK);
						mpi_fcache->close(fc);
					} else {
						p_httpc->res_code(HTTPRC_METHOD_NOT_ALLOWED);
						mpi_fcache->close(fc);
					}
				} else {
					p_httpc->res_code(HTTPRC_NOT_FOUND);
					call_handler(ON_NOT_FOUND, p_httpc);
				}
			} else
				p_httpc->res_code(HTTPRC_REQ_URI_TOO_LARGE);
		}
	}
}

void server::update_response(iHttpServerConnection *p_httpc) {
	// write response remainder
	HFCACHE fc = (HFCACHE)p_httpc->get_udata(IDX_FCACHE);

	if(fc) {
		// update content from file cache
		_ulong sz = 0;
		_u8 *ptr = (_u8 *)mpi_fcache->ptr(fc, &sz);

		if(ptr) {
			_u32 res_sent = p_httpc->res_content_sent();
			_u32 res_remainder = p_httpc->res_remainder();

			p_httpc->res_write(ptr + res_sent, res_remainder);
		}
	} else {
		// update content from response buffer
		_connection_t *pc = (_connection_t *)p_httpc->get_udata(IDX_CONNECTION);
		if(pc)
			pc->res.process_content();
	}
}

void server::on_route(_u8 method, _cstr_t path, _on_route_event_t *pcb, void *udata) {
	_route_key_t key;
	_route_data_t data = {pcb, udata};

	strncpy(data.path, path, sizeof(data.path)-1);

	memset(&key, 0, sizeof(_route_key_t));
	key.method = method;
	strncpy(key.path, path, MAX_ROUTE_PATH-1);
	if(mpi_map)
		mpi_map->add(&key, key.size(), &data, data.size());
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

_request_t *server::get_request(iHttpServerConnection *pi_httpc) {
	_request_t *r = NULL;
	_connection_t *pc = (_connection_t *)pi_httpc->get_udata(IDX_CONNECTION);

	if(pc)
		r = &pc->req;

	return r;
}

_response_t *server::get_response(iHttpServerConnection *pi_httpc) {
	_response_t *r = NULL;

	_connection_t *pc = (_connection_t *)pi_httpc->get_udata(IDX_CONNECTION);

	if(pc)
		r = &pc->res;

	return r;
}

void server::enum_route(void (*enum_cb)(_cstr_t path, _on_route_event_t *pcb, void *udata), void *udata) {
	_map_enum_t me = mpi_map->enum_open();
	_u32 sz = 0;
	HMUTEX hm = mpi_map->lock();

	if(me) {
		_route_data_t *rd = (_route_data_t *)mpi_map->enum_first(me, &sz, hm);

		while(rd) {
			mpi_map->unlock(hm);
			enum_cb(rd->path, rd->pcb, udata);
			hm = mpi_map->lock();
			rd = (_route_data_t *)mpi_map->enum_next(me, &sz, hm);
		}

		mpi_map->enum_close(me);
	}

	mpi_map->unlock(hm);
}

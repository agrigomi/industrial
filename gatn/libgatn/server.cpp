#include <string.h>
#include "iGatn.h"
#include "iRepository.h"
#include "private.h"

#define MAX_ROUTE_PATH	256

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
	_cstr_t		url;
	_vhost_t	*p_vhost;
	HFCACHE		hfc;

	void clear(void) {
		res.clear();
		url = NULL;
		hfc = NULL;
		p_vhost = NULL;
	}

	void destroy(void) {
		req.destroy();
		res.destroy();
		url = NULL;
		hfc = NULL;
		p_vhost = NULL;
	}
}_connection_t;

server::server(_cstr_t name, _u32 port, _cstr_t root,
		_cstr_t cache_path, _cstr_t cache_exclude,
		_u32 buffer_size,
		_u32 max_workers, _u32 max_connections,
		_u32 connection_timeout, SSL_CTX *ssl_context) {
	memset(&host, 0, sizeof(_vhost_t)); // default host
	mpi_server = NULL; // HTTP server
	mpi_vhost_map = NULL;
	strncpy(m_name, name, sizeof(m_name)-1);
	m_port = port;
	strncpy(host.root, root, sizeof(host.root)-1);
	strncpy(host.cache_path, cache_path, sizeof(host.cache_path)-1);
	strncpy(host.cache_key, m_name, sizeof(host.cache_key)-1);
	mpi_log = dynamic_cast<iLog *>(_gpi_repo_->object_by_iname(I_LOG, RF_ORIGINAL));
	mpi_heap = dynamic_cast<iHeap *>(_gpi_repo_->object_by_iname(I_HEAP, RF_ORIGINAL));
	mpi_net = NULL;
	mpi_fs = NULL;
	attach_fs();
	attach_network();
	m_autorestore = false;
	if((mpi_bmap = dynamic_cast<iBufferMap *>(_gpi_repo_->object_by_iname(I_BUFFER_MAP, RF_CLONE|RF_NONOTIFY)))) {
		mpi_bmap->init(buffer_size, [](_u8 op, void *bptr, _u32 sz, void *udata)->_u32 {
			_u32 r = 0;

			if(op == BIO_INIT)
				memset(bptr, 0, sz);

			return r;
		});
	}
	m_buffer_size = buffer_size;
	m_max_workers = max_workers;
	m_max_connections = max_connections;
	m_connection_timeout = connection_timeout;
	m_ssl_context = ssl_context;
}

void server::attach_network(void) {
	if(!mpi_net) {
		if((mpi_net = dynamic_cast<iNet *>(_gpi_repo_->object_by_iname(I_NET, RF_ORIGINAL))))
			mpi_log->fwrite(LMT_INFO, "'%s' attach networking", m_name);
		else
			mpi_log->fwrite(LMT_ERROR, "'%s' unable to attach networking", m_name);
	}
}
void server::attach_fs(void) {
	if(!mpi_fs) {
		if((mpi_fs = dynamic_cast<iFS *>(_gpi_repo_->object_by_iname(I_FS, RF_ORIGINAL))))
			mpi_log->fwrite(LMT_INFO, "'%s' attach FS", m_name);
		else
			mpi_log->fwrite(LMT_ERROR, "'%s' unable to attach FS", m_name);
	}
}

void server::release_network(void) {
	stop();
	if(mpi_net) {
		mpi_log->fwrite(LMT_INFO, "'%s' detach networking", m_name);
		_gpi_repo_->object_release(mpi_net);
		mpi_net = NULL;
	}
}

void server::release_fs(void) {
	stop();
	if(mpi_fs) {
		mpi_log->fwrite(LMT_INFO, "'%s' detach FS", m_name);
		_gpi_repo_->object_release(mpi_fs);
		mpi_fs = NULL;
	}
}

void server::destroy(void) {
	release_network();
	release_fs();

	// destroy virtual hosts
	enum_virtual_hosts([](_vhost_t *pvhost, void *udata) {
		server *p = (server *)udata;
		p->destroy(pvhost);
	}, this);

	destroy(&host); // destroy default host

	_gpi_repo_->object_release(mpi_vhost_map);
	_gpi_repo_->object_release(mpi_heap);
	_gpi_repo_->object_release(mpi_log);
	_gpi_repo_->object_release(mpi_bmap);
}

void server::destroy(_vhost_t *pvhost) {
	stop(pvhost);

	if(pvhost->pi_route_map) {
		_gpi_repo_->object_release(pvhost->pi_route_map);
		pvhost->pi_route_map = NULL;
	}
	if(pvhost->pi_nocache_map) {
		_gpi_repo_->object_release(pvhost->pi_nocache_map);
		pvhost->pi_nocache_map = NULL;
	}
}

bool server::create_connection(iHttpServerConnection *p_httpc) {
	bool r = false;
	_connection_t tmp;
	_connection_t *p_cnt = (_connection_t *)mpi_heap->alloc(sizeof(_connection_t));

	if(p_cnt) {
		memcpy((void *)p_cnt, (void *)&tmp, sizeof(_connection_t));
		p_cnt->req.mpi_httpc = p_cnt->res.mpi_httpc = p_httpc;
		p_cnt->req.mpi_server = p_cnt->res.mpi_server = this;
		p_cnt->res.mpi_heap = mpi_heap;
		p_cnt->res.mpi_bmap = mpi_bmap;
		p_cnt->res.mp_hbarray = NULL;
		p_cnt->res.m_hbcount = 0;
		p_cnt->res.m_content_len = 0;
		p_cnt->res.m_buffers = 0;
		p_cnt->res.m_doc_root = host.root;
		p_cnt->res.mpi_fcache = host.pi_fcache;
		p_cnt->res.mpi_fs = mpi_fs;
		p_cnt->clear();
		p_httpc->set_udata((_ulong)p_cnt, IDX_CONNECTION);
		r = true;
	}

	return r;
}

void server::destroy_connection(iHttpServerConnection *p_httpc) {
	_connection_t *pc = (_connection_t *)p_httpc->get_udata(IDX_CONNECTION);

	if(pc) {
		if(pc->hfc && pc->p_vhost && pc->p_vhost->pi_fcache)
			// close cache handle
			pc->p_vhost->pi_fcache->close(pc->hfc);

		// detach connection record
		p_httpc->set_udata((_ulong)NULL, IDX_CONNECTION);

		// destroy connection record
		pc->destroy();

		// release connection memory
		mpi_heap->free(pc, sizeof(_connection_t));
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
		_connection_t *pc = (_connection_t *)p_httpc->get_udata(IDX_CONNECTION);

		/************************************
		 Check for unclosed handle from file cache,
		 because in cases of reuse connection (connection: keep-alive),
		 HTTP_ON_CLOSE will not happened
		*/
		if(pc->hfc) {
			_vhost_t *p_vhost = pc->p_vhost;

			if(p_vhost && p_vhost->pi_fcache)
				p_vhost->pi_fcache->close(pc->hfc);
		}
		/**********************************/
		pc->clear();
		pc->p_vhost = p_srv->get_host(p_httpc->req_var("Host"));
		pc->url = p_httpc->req_url();

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

		p_srv->update_response(p_httpc);
	}, this);

	mpi_server->on_event(HTTP_ON_ERROR, [](iHttpServerConnection *p_httpc, void *udata) {
		server *p_srv = (server *)udata;

		p_srv->call_handler(HTTP_ON_ERROR, p_httpc);
		p_srv->call_route_handler(HTTP_ON_ERROR, p_httpc);
	}, this);

	mpi_server->on_event(HTTP_ON_CLOSE, [](iHttpServerConnection *p_httpc, void *udata) {
		server *p_srv = (server *)udata;

		p_srv->call_handler(HTTP_ON_CLOSE, p_httpc);
		p_srv->destroy_connection(p_httpc);
	}, this);
}

bool server::start(void) {
	bool r = false;

	attach_fs();
	attach_network();

	if(mpi_net && mpi_fs) {
		// start virtual hosts
		enum_virtual_hosts([](_vhost_t *pvhost, void *udata) {
			server *p = (server *)udata;
			p->start(pvhost);
		}, this);

		// start default host
		start(&host);

		if(!mpi_server) // create HTTP engine
			mpi_server = mpi_net->create_http_server(m_port, m_buffer_size, m_max_workers,
								m_max_connections, m_connection_timeout,
								m_ssl_context);

		if(host.pi_fcache && mpi_server) {
			set_handlers();
			r = true;
		}
	}

	return r;
}

void server::stop(_vhost_t *pvhost) {
	// release file cache
	if(pvhost->pi_fcache) {
		_gpi_repo_->object_release(pvhost->pi_fcache);
		pvhost->pi_fcache = NULL;
	}
}

_vhost_t *server::get_host(_cstr_t _host) {
	_vhost_t *r = (_host) ? get_virtual_host(_host) : &host;
	return (r) ? r : &host;
}

bool server::start(_vhost_t *pvhost) {
	bool r = false;

	if(!pvhost->pi_fcache) {
		if((pvhost->pi_fcache = dynamic_cast<iFileCache *>(_gpi_repo_->object_by_iname(I_FILE_CACHE, RF_CLONE|RF_NONOTIFY))))
			r = pvhost->pi_fcache->init(pvhost->cache_path, pvhost->cache_key);
	}

	return r;
}

void server::stop(void) {
	if(mpi_server) {
		_gpi_repo_->object_release(mpi_server, false);
		mpi_server = NULL;
	}

	enum_virtual_hosts([](_vhost_t *pvhost, void *udata) {
		server *p = (server *)udata;
		p->stop(pvhost);
	}, this);

	stop(&host); // stop default host
}

void server::call_handler(_u8 evt, iHttpServerConnection *p_httpc) {
	if(evt < HTTP_MAX_EVENTS) {
		if(host.event[evt].pcb)
			host.event[evt].pcb(p_httpc, host.event[evt].udata);
	}
}

_cstr_t server::resolve_content_type(_cstr_t doc_name) {
	_cstr_t r = resolve_mime_type(doc_name);
	return (r) ? r : "";
}

void server::call_route_handler(_u8 evt, iHttpServerConnection *p_httpc) {
	_route_key_t key;
	_u32 sz=0;
	_connection_t *pc = (_connection_t *)p_httpc->get_udata(IDX_CONNECTION);
	_vhost_t *pvhost = pc->p_vhost;
	_cstr_t url = pc->url;
	iFileCache *pi_fcache = pvhost->pi_fcache;

	if(pvhost && url) {
		memset(&key, 0, sizeof(_route_key_t));
		key.method = p_httpc->req_method();
		strncpy(key.path, url, sizeof(key.path)-1);

		_route_data_t *prd = (_route_data_t *)pvhost->pi_route_map->get(&key, key.size(), &sz);

		if(prd) {
			// route found
			prd->pcb(evt, &pc->req, &pc->res, prd->udata);
		} else if(evt == HTTP_ON_REQUEST) {
			// route not found
			// try to resolve file name
			_char_t doc[MAX_DOC_ROOT_PATH * 2]="";

			if((strlen(url) + strlen(pvhost->root) < sizeof(doc)-1)) {
				snprintf(doc, sizeof(doc), "%s%s",
					pvhost->root,
					(strcmp(url, "/") == 0) ? "/index.html" : url);
				HFCACHE fc = (pi_fcache) ? pi_fcache->open(doc) : NULL;
				if(fc) {
					if(key.method == HTTP_METHOD_GET) {
						_ulong doc_sz = 0;

						_u8 *ptr = (_u8 *)pi_fcache->ptr(fc, &doc_sz);
						if(ptr) {
							// response header
							p_httpc->res_content_len(doc_sz);
							p_httpc->res_code(HTTPRC_OK);
							p_httpc->res_var("Server", m_name);
							p_httpc->res_var("Content-Type", resolve_content_type(doc));
							p_httpc->res_mtime(pi_fcache->mtime(fc));
							// response content
							p_httpc->res_write(ptr, doc_sz);
							pc->hfc = fc;
						} else {
							p_httpc->res_code(HTTPRC_INTERNAL_SERVER_ERROR);
							pi_fcache->close(fc);
						}
					} else if(key.method == HTTP_METHOD_HEAD) {
						p_httpc->res_mtime(pi_fcache->mtime(fc));
						p_httpc->res_code(HTTPRC_OK);
						pi_fcache->close(fc);
					} else {
						p_httpc->res_code(HTTPRC_METHOD_NOT_ALLOWED);
						pi_fcache->close(fc);
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
	_connection_t *pc = (_connection_t *)p_httpc->get_udata(IDX_CONNECTION);

	if(pc->hfc) {
		// update content from file cache
		_ulong sz = 0;
		_vhost_t *pvhost = pc->p_vhost;

		if(pvhost) {
			_u8 *ptr = (_u8 *)pvhost->pi_fcache->ptr(pc->hfc, &sz);

			if(ptr) {
				_u32 res_sent = p_httpc->res_content_sent();
				_u32 res_remainder = p_httpc->res_remainder();

				p_httpc->res_write(ptr + res_sent, res_remainder);
			}
		}
	} else
		// update content from response buffer
		pc->res.process_content();
}

void server::on_route(_u8 method, _cstr_t path, _on_route_event_t *pcb, void *udata) {
	_route_key_t key;
	_route_data_t data = {pcb, udata};

	strncpy(data.path, path, sizeof(data.path)-1);

	memset(&key, 0, sizeof(_route_key_t));
	key.method = method;
	strncpy(key.path, path, MAX_ROUTE_PATH-1);
	if(!host.pi_route_map) {
		if((host.pi_route_map = dynamic_cast<iMap *>(_gpi_repo_->object_by_iname(I_MAP, RF_CLONE|RF_NONOTIFY))))
			host.pi_route_map->init(33, mpi_heap);
	}
	if(host.pi_route_map)
		host.pi_route_map->add(&key, key.size(), &data, data.size());
}

void server::on_event(_u8 evt, _on_http_event_t *pcb, void *udata) {
	if(evt < HTTP_MAX_EVENTS) {
		host.event[evt].pcb = pcb;
		host.event[evt].udata = udata;
	}
}

void server::remove_route(_u8 method, _cstr_t path) {
	_route_key_t key;

	memset(&key, 0, sizeof(_route_key_t));
	key.method = method;
	strncpy(key.path, path, MAX_ROUTE_PATH-1);
	if(host.pi_route_map)
		host.pi_route_map->del(&key, sizeof(_route_key_t));
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
	_map_enum_t me = host.pi_route_map->enum_open();
	_u32 sz = 0;
	HMUTEX hm = host.pi_route_map->lock();

	if(me) {
		_route_data_t *rd = (_route_data_t *)host.pi_route_map->enum_first(me, &sz, hm);

		while(rd) {
			host.pi_route_map->unlock(hm);
			enum_cb(rd->path, rd->pcb, udata);
			hm = host.pi_route_map->lock();
			rd = (_route_data_t *)host.pi_route_map->enum_next(me, &sz, hm);
		}

		host.pi_route_map->enum_close(me);
	}

	host.pi_route_map->unlock(hm);
}

bool server::add_virtual_host(_cstr_t host, _cstr_t root, _cstr_t cache_path, _cstr_t cache_key, _cstr_t cache_exclude) {
	bool r = false;
	_vhost_t vhost;

	strncpy(vhost.host, host, sizeof(vhost.host)-1);
	strncpy(vhost.root, root, sizeof(vhost.root)-1);
	strncpy(vhost.cache_path, cache_path, sizeof(vhost.cache_path)-1);
	strncpy(vhost.cache_key, cache_key, sizeof(vhost.cache_key)-1);

	if(!mpi_vhost_map) {
		if((mpi_vhost_map = dynamic_cast<iMap *>(_gpi_repo_->object_by_iname(I_MAP, RF_CLONE|RF_NONOTIFY))))
			mpi_vhost_map->init(15, mpi_heap);
	}

	if(mpi_vhost_map) {
		if(mpi_vhost_map->add(host, strlen(host), &vhost, sizeof(_vhost_t)))
			r = true;
	}

	return r;
}

_vhost_t *server::get_virtual_host(_cstr_t host) {
	_u32 sz = 0;
	_vhost_t *r = NULL;

	if(mpi_vhost_map)
		r = (_vhost_t *)mpi_vhost_map->get(host, strlen(host), &sz);

	return r;
}

bool server::remove_virtual_host(_cstr_t host) {
	bool r = false;
	_u32 sz = 0;

	if(mpi_vhost_map) {
		HMUTEX hm = mpi_vhost_map->lock();
		_vhost_t *pvhost = (_vhost_t *)mpi_vhost_map->get(host, strlen(host), &sz, hm);

		if(pvhost) {
			destroy(pvhost);
			mpi_vhost_map->del(host, strlen(host), hm);
			r = true;
		}

		mpi_vhost_map->unlock(hm);
	}

	return r;
}

bool server::start_virtual_host(_cstr_t host) {
	bool r = false;
	_u32 sz = 0;

	if(mpi_vhost_map) {
		HMUTEX hm = mpi_vhost_map->lock();
		_vhost_t *pvhost = (_vhost_t *)mpi_vhost_map->get(host, strlen(host), &sz, hm);

		if(pvhost) {
			start(pvhost);
			r = true;
		}

		mpi_vhost_map->unlock(hm);
	}

	return r;
}

bool server::stop_virtual_host(_cstr_t host) {
	bool r = false;
	_u32 sz = 0;

	if(mpi_vhost_map) {
		HMUTEX hm = mpi_vhost_map->lock();
		_vhost_t *pvhost = (_vhost_t *)mpi_vhost_map->get(host, strlen(host), &sz, hm);

		if(pvhost) {
			stop(pvhost);
			r = true;
		}

		mpi_vhost_map->unlock(hm);
	}

	return r;
}

void server::enum_virtual_hosts(void (*enum_cb)(_vhost_t *, void *udata), void *udata) {
	if(mpi_vhost_map) {
		_map_enum_t vhe = mpi_vhost_map->enum_open();
		HMUTEX hm = mpi_vhost_map->lock();
		_u32 sz = 0;
		_vhost_t *pvhost = (_vhost_t *)mpi_vhost_map->enum_first(vhe, &sz, hm);

		while(pvhost) {
			enum_cb(pvhost, udata);
			pvhost = (_vhost_t *)mpi_vhost_map->enum_next(vhe, &sz, hm);
		}

		mpi_vhost_map->unlock(hm);
		mpi_vhost_map->enum_close(vhe);
	}
}

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
	HDOCUMENT	hdoc;

	void clear(void) {
		if(hdoc && p_vhost) // close handle
			p_vhost->root.close(hdoc);
		res.clear();
		url = NULL;
		hdoc = NULL;
		p_vhost = NULL;
	}

	void destroy(void) {
		if(hdoc && p_vhost) // close handle
			p_vhost->root.close(hdoc);
		req.destroy();
		res.destroy();
		url = NULL;
		hdoc = NULL;
		p_vhost = NULL;
	}
}_connection_t;

bool server::init(_cstr_t name, _u32 port, _cstr_t root,
		_cstr_t cache_path, _cstr_t cache_exclude,
		_cstr_t path_disable, _u32 buffer_size,
		_u32 max_workers, _u32 max_connections,
		_u32 connection_timeout, SSL_CTX *ssl_context) {
	bool r = false;

	memset(&host, 0, sizeof(_vhost_t)); // default host
	strncpy(host.host, "defulthost", sizeof(host.host));
	mpi_server = NULL; // HTTP server
	mpi_vhost_map = NULL;
	strncpy(m_name, name, sizeof(m_name)-1);
	m_port = port;
	mpi_log = dynamic_cast<iLog *>(_gpi_repo_->object_by_iname(I_LOG, RF_ORIGINAL));
	mpi_heap = dynamic_cast<iHeap *>(_gpi_repo_->object_by_iname(I_HEAP, RF_ORIGINAL));
	if((mpi_pool = dynamic_cast<iPool *>(_gpi_repo_->object_by_iname(I_POOL, RF_CLONE|RF_NONOTIFY)))) {
		mpi_pool->init(sizeof(_connection_t), [](_u8 op, void *data, void *udata) {
			_connection_t *pcnt = (_connection_t *)data;
			server *p_srv = (server *)udata;

			switch(op) {
				case POOL_OP_NEW:
				case POOL_OP_BUSY: {
						_connection_t tmp;

						tmp.req.mpi_server = tmp.res.mpi_server = p_srv;
						tmp.res.mpi_heap = p_srv->mpi_heap;
						tmp.res.mpi_bmap = p_srv->mpi_bmap;
						tmp.res.mp_hbarray = NULL;
						tmp.res.m_hbcount = 0;
						tmp.res.m_buffers = 0;
						tmp.res.m_content_len = 0;
						tmp.res.mpi_fs = p_srv->mpi_fs;
						tmp.url = NULL;
						tmp.hdoc = NULL;
						tmp.p_vhost = NULL;
						memcpy((void *)pcnt, (void *)&tmp, sizeof(_connection_t));
					} break;
				case POOL_OP_FREE:
					break;
				case POOL_OP_DELETE:
					pcnt->destroy();
					break;
			};
		}, this, mpi_heap);
	}
	mpi_net = NULL;
	mpi_fs = NULL;
	attach_fs();
	attach_network();
	m_autorestore = false;
	if((mpi_bmap = dynamic_cast<iBufferMap *>(_gpi_repo_->object_by_iname(I_BUFFER_MAP, RF_CLONE|RF_NONOTIFY)))) {
		mpi_bmap->init(m_buffer_size, [](_u8 op, void *bptr, _u32 sz, void *udata)->_u32 {
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

	r = host.root.init(root, cache_path, name, cache_exclude, path_disable, mpi_heap);

	return r;
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

	_gpi_repo_->object_release(mpi_vhost_map, false);
	_gpi_repo_->object_release(mpi_heap);
	_gpi_repo_->object_release(mpi_log);
	_gpi_repo_->object_release(mpi_bmap, false);
	_gpi_repo_->object_release(mpi_pool, false);
}

void server::destroy(_vhost_t *pvhost) {
	stop(pvhost);

	if(pvhost->pi_class_map) {
		// detach extensions
		_map_enum_t me = pvhost->pi_class_map->enum_open();

		if(me) {
			_u32 sz = 0;
			iBase **ppi_base = (iBase **)pvhost->pi_class_map->enum_first(me, &sz);

			while(ppi_base) {
				_object_info_t oi;

				(*ppi_base)->object_info(&oi);
				detach_class(oi.cname, pvhost->host);
				ppi_base = (iBase **)pvhost->pi_class_map->enum_next(me, &sz);
			}

			pvhost->pi_class_map->enum_close(me);
		}

		_gpi_repo_->object_release(pvhost->pi_class_map, false);
		pvhost->pi_class_map = NULL;
	}

	if(pvhost->pi_route_map) {
		// destroy routing
		_gpi_repo_->object_release(pvhost->pi_route_map, false);
		pvhost->pi_route_map = NULL;
	}

	pvhost->root.destroy();
}

bool server::create_connection(iHttpServerConnection *p_httpc) {
	bool r = false;

	_connection_t *p_cnt = (_connection_t *)mpi_pool->alloc();

	if(p_cnt) {
		p_cnt->req.mpi_httpc = p_cnt->res.mpi_httpc = p_httpc;
		p_httpc->set_udata((_ulong)p_cnt, IDX_CONNECTION);
		r = true;
	}

	return r;
}

void server::destroy_connection(iHttpServerConnection *p_httpc) {
	_connection_t *pc = (_connection_t *)p_httpc->get_udata(IDX_CONNECTION);

	if(pc) {
		// detach connection record
		p_httpc->set_udata((_ulong)NULL, IDX_CONNECTION);

		if(pc->p_vhost && pc->hdoc)
			// close document handle
			pc->p_vhost->root.close(pc->hdoc);

		// destroy connection record
		pc->destroy();

		// release connection memory
		mpi_pool->free(pc);
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

		pc->clear();
		pc->p_vhost = p_srv->get_host(p_httpc->req_var("Host"));
		pc->url = p_httpc->req_url();
		pc->res.m_doc_root = pc->p_vhost->root.get_doc_root();
		pc->res.mpi_fcache = pc->p_vhost->root.get_file_cache();

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

		if(host.root.is_enabled() && mpi_server) {
			mpi_log->fwrite(LMT_INFO, "Gatn: Start server '%s'", m_name);
			set_handlers();
			r = true;
		}
	}

	return r;
}

void server::stop(_vhost_t *pvhost) {
	// release file cache
	if(pvhost->root.is_enabled()) {
		mpi_log->fwrite(LMT_INFO, "Gatn: Stop host '%s' on server '%s'", pvhost->host, m_name);
		pvhost->root.stop();
	}
}

_vhost_t *server::get_host(_cstr_t _host) {
	_vhost_t *r = (_host) ? get_virtual_host(_host) : &host;
	return (r) ? r : &host;
}

bool server::start(_vhost_t *pvhost) {
	bool r = false;

	if(!(r = pvhost->root.is_enabled())) {
		mpi_log->fwrite(LMT_INFO, "Gatn: Start host '%s' on server '%s'", pvhost->host, m_name);
		pvhost->root.start();
		r = true;
	}

	return r;
}

void server::stop(void) {
	if(mpi_server) {
		mpi_log->fwrite(LMT_INFO, "Gatn: Stop server '%s'", m_name);
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
	_connection_t *pc = (_connection_t *)p_httpc->get_udata(IDX_CONNECTION);
	_vhost_t *pvhost = (pc->p_vhost) ? pc->p_vhost : &host;

	if(evt < HTTP_MAX_EVENTS) {
		if(pvhost->event[evt].pcb)
			pvhost->event[evt].pcb(p_httpc, pvhost->event[evt].udata);
	}
}

void server::call_route_handler(_u8 evt, iHttpServerConnection *p_httpc) {
	_u32 sz=0;
	_connection_t *pc = (_connection_t *)p_httpc->get_udata(IDX_CONNECTION);
	_vhost_t *pvhost = pc->p_vhost;
	_cstr_t url = pc->url;

	if(pvhost && url) {
		_root_t *root = &pvhost->root;
		_route_data_t *prd = NULL;
		_u8 method = p_httpc->req_method();

		if(pvhost->pi_route_map) {
			_route_key_t key;

			memset(&key, 0, sizeof(_route_key_t));
			key.method = method;
			strncpy(key.path, url, sizeof(key.path)-1);

			prd = (_route_data_t *)pvhost->pi_route_map->get(&key, key.size(), &sz);
		}

		if(prd)
			// route found
			prd->pcb(evt, &pc->req, &pc->res, prd->udata);
		else if(root->is_enabled() && evt == HTTP_ON_REQUEST) {
			if(method == HTTP_METHOD_GET || method == HTTP_METHOD_HEAD) {
				// route not found
				// try to resolve file name
				HDOCUMENT hdoc = root->open(url);
				if(hdoc) {
					if(method == HTTP_METHOD_GET) {
						_ulong doc_sz = 0;

						_u8 *ptr = (_u8 *)root->ptr(hdoc, &doc_sz);
						if(ptr) {
							// response header
							p_httpc->res_content_len(doc_sz);
							p_httpc->res_code(HTTPRC_OK);
							p_httpc->res_var("Server", m_name);
							p_httpc->res_var("Content-Type", root->mime(hdoc));
							p_httpc->res_protocol("HTTP/2.0");
							p_httpc->res_mtime(root->mtime(hdoc));
							// response content
							p_httpc->res_write(ptr, doc_sz);
							pc->hdoc = hdoc;
						} else {
							p_httpc->res_code(HTTPRC_INTERNAL_SERVER_ERROR);
							call_handler(ON_ERROR, p_httpc);
							root->close(hdoc);
						}
					} else if(method == HTTP_METHOD_HEAD) {
						p_httpc->res_mtime(root->mtime(hdoc));
						p_httpc->res_code(HTTPRC_OK);
						root->close(hdoc);
					}
				} else {
					p_httpc->res_code(HTTPRC_NOT_FOUND);
					call_handler(ON_NOT_FOUND, p_httpc);
				}
			} else {
				p_httpc->res_code(HTTPRC_METHOD_NOT_ALLOWED);
				call_handler(ON_ERROR, p_httpc);
			}
		}
	}
}

void server::update_response(iHttpServerConnection *p_httpc) {
	// write response remainder
	_connection_t *pc = (_connection_t *)p_httpc->get_udata(IDX_CONNECTION);

	if(pc->hdoc) {
		// update content from file cache
		_ulong sz = 0;
		_vhost_t *pvhost = pc->p_vhost;

		if(pvhost) {
			_u8 *ptr = (_u8 *)pvhost->root.ptr(pc->hdoc, &sz);

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

void server::on_route(_u8 method, _cstr_t path, _on_route_event_t *pcb, void *udata, _cstr_t _host) {
	_route_key_t key;
	_route_data_t data = {pcb, udata};
	_vhost_t *pvhost = get_host(_host);

	strncpy(data.path, path, sizeof(data.path)-1);

	memset(&key, 0, sizeof(_route_key_t));
	key.method = method;
	strncpy(key.path, path, MAX_ROUTE_PATH-1);
	if(!pvhost->pi_route_map) {
		if((pvhost->pi_route_map = dynamic_cast<iMap *>(_gpi_repo_->object_by_iname(I_MAP, RF_CLONE|RF_NONOTIFY))))
			pvhost->pi_route_map->init(33, mpi_heap);
	}
	if(pvhost->pi_route_map)
		pvhost->pi_route_map->add(&key, key.size(), &data, data.size());
}

void server::on_event(_u8 evt, _on_http_event_t *pcb, void *udata, _cstr_t _host) {
	_vhost_t *pvhost = get_host(_host);

	if(evt < HTTP_MAX_EVENTS) {
		pvhost->event[evt].pcb = pcb;
		pvhost->event[evt].udata = udata;
	}
}

_on_http_event_t *server::get_event_handler(_u8 evt, void **pp_udata, _cstr_t host) {
	_on_http_event_t *r = NULL;
	_vhost_t *pvhost = get_host(host);

	if(evt < HTTP_MAX_EVENTS) {
		r = pvhost->event[evt].pcb;
		*pp_udata = pvhost->event[evt].udata;
	}

	return r;
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

	if(me) {
		_route_data_t *rd = (_route_data_t *)host.pi_route_map->enum_first(me, &sz);

		while(rd) {
			enum_cb(rd->path, rd->pcb, udata);
			rd = (_route_data_t *)host.pi_route_map->enum_next(me, &sz);
		}

		host.pi_route_map->enum_close(me);
	}
}

bool server::add_virtual_host(_cstr_t host, _cstr_t root, _cstr_t cache_path, _cstr_t cache_key,
				_cstr_t cache_exclude, _cstr_t path_disable) {
	bool r = false;
	_vhost_t vhost;

	memset(&vhost, 0, sizeof(_vhost_t));
	strncpy(vhost.host, host, sizeof(vhost.host)-1);

	if(!mpi_vhost_map) {
		if((mpi_vhost_map = dynamic_cast<iMap *>(_gpi_repo_->object_by_iname(I_MAP, RF_CLONE|RF_NONOTIFY))))
			mpi_vhost_map->init(15, mpi_heap);
	}

	if(mpi_vhost_map) {
		_vhost_t *pvhost = (_vhost_t *)mpi_vhost_map->add(host, strlen(host), &vhost, sizeof(_vhost_t));

		if(pvhost)
			r = pvhost->root.init(root, cache_path, cache_key,
						cache_exclude, path_disable, mpi_heap);
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

		if(pvhost)
			r = start(pvhost);

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
		_u32 sz = 0;
		_vhost_t *pvhost = (_vhost_t *)mpi_vhost_map->enum_first(vhe, &sz);

		while(pvhost) {
			enum_cb(pvhost, udata);
			pvhost = (_vhost_t *)mpi_vhost_map->enum_next(vhe, &sz);
		}

		mpi_vhost_map->enum_close(vhe);
	}
}

bool server::attach_class(_cstr_t cname, _cstr_t options, _cstr_t _host) {
	bool r = false;
	_u32 sz = 0;
	_vhost_t *pvhost = get_host(_host);

	if(pvhost->pi_class_map == NULL) {
		if((pvhost->pi_class_map = dynamic_cast<iMap *>(_gpi_repo_->object_by_iname(I_MAP, RF_CLONE|RF_NONOTIFY))))
			pvhost->pi_class_map->init(15, mpi_heap);
	}

	if(pvhost->pi_class_map) {
		iGatnExtension **ppi_ext = (iGatnExtension **)pvhost->pi_class_map->get(cname, strlen(cname), &sz);

		if(!ppi_ext) {
			_object_info_t oi;
			iBase *pi_base = _gpi_repo_->object_by_cname(cname, RF_CLONE|RF_NONOTIFY);

			if(pi_base) {
				pi_base->object_info(&oi);

				if(strcmp(I_GATN_EXTENSION, oi.iname) == 0) {
					iGatnExtension *pi_ext = dynamic_cast<iGatnExtension *>(pi_base);

					if(pi_ext) {
						if(pvhost->pi_class_map->add(cname, strlen(cname), &pi_ext, sizeof(pi_ext))) {
							mpi_log->fwrite(LMT_INFO, "Gatn: Attach class '%s' to '%s/%s'",
									cname, m_name, pvhost->host);
							if(options)
								pi_ext->options(options);
							r = pi_ext->attach(this, _host);
						} else
							_gpi_repo_->object_release(pi_ext, false);
					}
				} else {
					mpi_log->fwrite(LMT_ERROR, "Gatn: Unable to attach class '%s'", cname);
					_gpi_repo_->object_release(pi_base, false);
				}
			} else
				mpi_log->fwrite(LMT_ERROR, "Gatn: Unable to clone class '%s'", cname);
		} else
			mpi_log->fwrite(LMT_ERROR, "Gatn: class '%s' already attached to '%s/%s'",
					cname, m_name, pvhost->host);
	}

	return r;
}

bool server::detach_class(_cstr_t cname, _cstr_t _host) {
	bool r = false;
	_u32 sz = 0;
	_vhost_t *pvhost = get_host(_host);

	if(pvhost->pi_class_map) {
		iGatnExtension **ppi_ext = (iGatnExtension **)pvhost->pi_class_map->get(cname, strlen(cname), &sz);

		if(ppi_ext) {
			_object_info_t oi;

			(*ppi_ext)->object_info(&oi);
			mpi_log->fwrite(LMT_INFO, "Gatn: Detach class '%s' from '%s/%s'",
					cname, m_name, pvhost->host);
			(*ppi_ext)->detach(this, _host);
			_gpi_repo_->object_release(*ppi_ext, false);
			pvhost->pi_class_map->del(cname, strlen(cname));
			r = true;
		}
	}

	return r;
}
void server::release_class(_cstr_t cname) {
	struct _xhost {
		_cstr_t _cname;
		server	*psrv;
	};

	_xhost tmp { cname, this};

	enum_virtual_hosts([](_vhost_t *pvhost, void *udata) {
		_xhost *p = (_xhost *)udata;

		p->psrv->detach_class(p->_cname, pvhost->host);
	}, &tmp);

	detach_class(cname);
}

#include <string.h>
#include <string>
#include "startup.h"
#include "iGatn.h"
#include "iLog.h"
#include "iHT.h"
#include "private.h"

IMPLEMENT_BASE_ARRAY("libgatn", 10);

class cGatn: public iGatn {
private:
	iMap		*mpi_map;// server map
	iLog		*mpi_log;
	_char_t		m_config_fname[256];
	iJSON		*mpi_json;

	void stop(bool autorestore=false) { // stop servers
		_map_enum_t en = mpi_map->enum_open();

		if(en) {
			_u32 sz = 0;
			HMUTEX hm = mpi_map->lock();
			_server_t *p_srv = (_server_t *)mpi_map->enum_first(en, &sz, hm);

			while(p_srv) {
				server *p = dynamic_cast<server *>(p_srv);

				if(p) {
					p->release_network();
					p->release_fs();
					p->m_autorestore = autorestore;
					p_srv = (_server_t *)mpi_map->enum_next(en, &sz, hm);
				}
			}

			mpi_map->unlock(hm);
			mpi_map->enum_close(en);
		}
	}

	void restore(void) {
		_map_enum_t en = mpi_map->enum_open();

		if(en) {
			_u32 sz = 0;
			HMUTEX hm = mpi_map->lock();
			_server_t *p_srv = (_server_t *)mpi_map->enum_first(en, &sz, hm);

			while(p_srv) {
				server *p = dynamic_cast<server *>(p_srv);

				if(p) {
					p->attach_fs();
					p->attach_network();
					if(p->m_autorestore) {
						if(!p->start())
							mpi_log->fwrite(LMT_ERROR, "Gatn: Unable to restore server '%s'", p->name());
					}
					p_srv = (_server_t *)mpi_map->enum_next(en, &sz, hm);
				}
			}

			mpi_map->unlock(hm);
			mpi_map->enum_close(en);
		}
	}

	void destroy(_server_t *p_srv) {
		server *p = dynamic_cast<server *>(p_srv);

		if(p) {
			SSL_CTX *sslcxt = p->get_SSL_context();

			p->destroy();

			if(sslcxt) // destroy SSL context
				SSL_CTX_free(sslcxt);
		}
	}

	void destroy(void) {
		_map_enum_t en = mpi_map->enum_open();

		if(en) {
			_u32 sz = 0;
			HMUTEX hm = mpi_map->lock();
			_server_t *p_srv = (_server_t *)mpi_map->enum_first(en, &sz, hm);

			while(p_srv) {
				destroy(p_srv);
				p_srv = (_server_t *)mpi_map->enum_next(en, &sz, hm);
			}

			mpi_map->unlock(hm);
			mpi_map->enum_close(en);
		}
	}

	std::string json_string(HTVALUE htv) {
		std::string r;
		_u8 jvt = mpi_json->type(htv);

		if(jvt == JVT_STRING || jvt == JVT_NUMBER) {
			_u32 size = 0;
			_cstr_t data = mpi_json->data(htv, &size);

			r.assign(data, size);
		}

		return r;
	}

	std::string json_string(HTCONTEXT jcxt, _cstr_t var, HTVALUE parent=NULL) {
		std::string r;
		HTVALUE htv = mpi_json->select(jcxt, var, parent);

		if(htv)
			r = json_string(htv);

		return r;
	}

	std::string json_string(HTVALUE parent, _u32 idx) {
		std::string r;
		HTVALUE htv = mpi_json->by_index(parent, idx);

		if(htv)
			r = json_string(htv);

		return r;
	}

	std::string json_array_to_path(HTVALUE jarray) {
		std::string r;

		if(jarray) {
			if(mpi_json->type(jarray) == JVT_ARRAY) {
				HTVALUE item = NULL;
				_u32 idx = 0;

				while((item = mpi_json->by_index(jarray, idx))) {
					std::string tmp;

					if(mpi_json->type(item) == JVT_STRING) {
						tmp = json_string(item);
						r.append(tmp);
						r.append(":");
					}

					idx++;
				}
			} else if(mpi_json->type(jarray) == JVT_STRING)
				r = json_string(jarray);
		}

		return r;
	}

	void load_ssl_cert(HTCONTEXT jcxt, SSL_CTX *ssl_cxt, HTVALUE htv_ssl) {
		std::string cert = json_string(jcxt, "certificate", htv_ssl);
		std::string key = json_string(jcxt, "key", htv_ssl);

		if(SSL_CTX_use_certificate_file(ssl_cxt, cert.c_str(), SSL_FILETYPE_PEM) > 0) {
			if(SSL_CTX_use_PrivateKey_file(ssl_cxt, key.c_str(), SSL_FILETYPE_PEM) > 0) {
				if(!SSL_CTX_check_private_key(ssl_cxt))
					mpi_log->write(LMT_ERROR, "Private key does not match the public certificate");
			} else
				mpi_log->fwrite(LMT_ERROR, "Gatn: Unable to load private key '%s'", key.c_str());
		} else
			mpi_log->fwrite(LMT_ERROR, "Gatn: Unable to load certificate '%s'", cert.c_str());
	}

	SSL_CTX *create_ssl_context(HTCONTEXT jcxt, HTVALUE htv_srv) {
		SSL_CTX *r = NULL;
		HTVALUE htv_ssl = mpi_json->select(jcxt, "ssl", htv_srv);

		if(htv_ssl) {
			HTVALUE htv_ssl_enable = mpi_json->select(jcxt, "enable", htv_ssl);

			if(htv_ssl_enable && mpi_json->type(htv_ssl_enable) == JVT_TRUE) {
				std::string method = json_string(jcxt, "method", htv_ssl);
				const SSL_METHOD *ssl_method = ssl_select_method(method.c_str());

				if(ssl_method) {
					if((r = ssl_create_context(ssl_method)))
						load_ssl_cert(jcxt, r, htv_ssl);
					else
						mpi_log->fwrite(LMT_ERROR, "Gatn: '%s'", ssl_error_string());
				} else
					mpi_log->fwrite(LMT_ERROR, "Gatn: '%s'",ssl_error_string());
			}
		}

		return r;
	}

	void attach_class(HTCONTEXT jcxt, HTVALUE htv_class_array, _server_t *pi_srv, _cstr_t host=NULL) {
		HTVALUE htv_class = NULL;
		_u32 idx = 0;

		if(htv_class_array) {
			while((htv_class = mpi_json->by_index(htv_class_array, idx))) {
				std::string options = json_string(jcxt, "options", htv_class);
				std::string cname = json_string(jcxt, "cname", htv_class);

				pi_srv->attach_class(cname.c_str(), options.c_str(), host);

				idx++;
			}
		}
	}

	void load_extensions(HTCONTEXT jcxt) {
		HTVALUE htv_ext_array = mpi_json->select(jcxt, "extension", NULL);

		if(htv_ext_array) {
			if(mpi_json->type(htv_ext_array) == JVT_ARRAY) {
				_u32 idx = 0;
				HTVALUE htv_ext = NULL;

				while((htv_ext = mpi_json->by_index(htv_ext_array, idx))) {
					std::string module = json_string(jcxt, "module", htv_ext);
					std::string alias = json_string(jcxt, "alias", htv_ext);

					_gpi_repo_->extension_load(module.c_str(), alias.c_str());

					idx++;
				}
			} else
				mpi_log->write(LMT_ERROR, "Gatn: Requres array 'extension: []'");
		}
	}

	void configure_hosts(HTCONTEXT jcxt, HTVALUE htv_server, _server_t *pi_srv) {
		HTVALUE htv_vhost_array = mpi_json->select(jcxt, "vhost", htv_server);

		if(htv_vhost_array) {
			if(mpi_json->type(htv_vhost_array) == JVT_ARRAY) {
				HTVALUE htv_vhost = NULL;
				_u32 idx = 0;

				while((htv_vhost = mpi_json->by_index(htv_vhost_array, idx))) {
					std::string host = json_string(jcxt, "host", htv_vhost);
					std::string root = json_string(jcxt, "root", htv_vhost);
					std::string root_exclude = json_array_to_path(mpi_json->select(jcxt, "exclude", htv_vhost));
					std::string cache_path = json_string(jcxt, "cache.path", htv_vhost);
					std::string cache_key = json_string(jcxt, "cache.key", htv_vhost);
					std::string cache_exclude = json_array_to_path(mpi_json->select(jcxt, "cache.exclude", htv_vhost));

					if(pi_srv->add_virtual_host(host.c_str(), root.c_str(),
								cache_path.c_str(), cache_key.c_str(),
								cache_exclude.c_str(), root_exclude.c_str())) {
						if(pi_srv->start_virtual_host(host.c_str())) {
							HTVALUE htv_class_array = mpi_json->select(jcxt, "attach", htv_vhost);

							attach_class(jcxt, htv_class_array, pi_srv, host.c_str());
						}
					}

					idx++;
				}
			} else
				mpi_log->write(LMT_ERROR, "Gatn: Requires array 'server.vhost: []'");
		}
	}

	void configure_servers(HTCONTEXT jcxt) {
		HTVALUE htv_srv_array = mpi_json->select(jcxt, "server", NULL);

		if(htv_srv_array) {
			if(mpi_json->type(htv_srv_array) == JVT_ARRAY) {
				_u32 idx = 0;
				HTVALUE htv_srv = NULL;

				while((htv_srv = mpi_json->by_index(htv_srv_array, idx))) {
					std::string name = json_string(jcxt, "name", htv_srv);
					std::string root = json_string(jcxt, "root", htv_srv);
					std::string port = json_string(jcxt, "port", htv_srv);
					std::string buffer = json_string(jcxt, "buffer", htv_srv);
					std::string threads = json_string(jcxt, "threads", htv_srv);
					std::string connections = json_string(jcxt, "connections", htv_srv);
					std::string timeout = json_string(jcxt, "timeout", htv_srv);
					std::string cache_path = json_string(jcxt, "cache.path", htv_srv);
					std::string cache_key = json_string(jcxt, "cache.key", htv_srv);
					std::string cache_exclude = json_array_to_path(mpi_json->select(jcxt, "cache.exclude", htv_srv));
					std::string root_exclude = json_array_to_path(mpi_json->select(jcxt, "exclude", htv_srv));

					SSL_CTX *ssl_context = create_ssl_context(jcxt, htv_srv);

					_server_t *pi_srv = create_server(name.c_str(),
								atoi(port.c_str()),
								root.c_str(),
								cache_path.c_str(),
								cache_exclude.c_str(),
								root_exclude.c_str(),
								atoi(buffer.c_str()) * 1024,
								atoi(threads.c_str()),
								atoi(connections.c_str()),
								atoi(timeout.c_str()),
								ssl_context);

					if(pi_srv) {
						HTVALUE htv_class_array = mpi_json->select(jcxt, "attach", htv_srv);

						attach_class(jcxt, htv_class_array, pi_srv);
						configure_hosts(jcxt, htv_srv, pi_srv);
					}

					idx++;
				}
			} else
				mpi_log->write(LMT_ERROR, "Gatn: Requres array 'server: []'");
		} else
			mpi_log->write(LMT_ERROR, "Gatn: Failed to configure servers");
	}


public:
	BASE(cGatn, "cGatn", RF_ORIGINAL, 1,0,0);

	bool object_ctl(_u32 cmd, void *arg, ...) {
		bool r = false;

		switch(cmd) {
			case OCTL_INIT: {
				iRepository *pi_repo = (iRepository *)arg;

				if((mpi_map = dynamic_cast<iMap *>(pi_repo->object_by_iname(I_MAP, RF_CLONE|RF_NONOTIFY))))
					mpi_map->init(31);
				mpi_log = dynamic_cast<iLog *>(pi_repo->object_by_iname(I_LOG, RF_ORIGINAL));
				init_mime_type_resolver();
				if(mpi_map && mpi_log) {
					pi_repo->monitoring_add(NULL, I_NET, NULL, this, SCAN_ORIGINAL);
					pi_repo->monitoring_add(NULL, I_FS, NULL, this, SCAN_ORIGINAL);
					pi_repo->monitoring_add(NULL, I_GATN_EXTENSION, NULL, this);
					// init SSL
					ssl_init();
					r = true;
				}
			} break;
			case OCTL_UNINIT: {
				iRepository *pi_repo = (iRepository *)arg;

				destroy();
				pi_repo->object_release(mpi_map, false);
				pi_repo->object_release(mpi_log);
				uninit_mime_type_resolver();
				r = true;
			} break;
			case OCTL_NOTIFY: {
				_notification_t *pn = (_notification_t *)arg;
				_object_info_t oi;

				memset(&oi, 0, sizeof(_object_info_t));
				if(pn->object) {
					pn->object->object_info(&oi);

					if(pn->flags & NF_LOAD) {
						if(strcmp(oi.iname, I_GATN_EXTENSION) == 0) {
							// register extension
							mpi_log->fwrite(LMT_INFO, "Gatn: register class '%s'", oi.cname);
							//...
						}
					}
					if(pn->flags & NF_INIT) { // catch
						if(strcmp(oi.iname, I_NET) == 0) {
							mpi_log->write(LMT_INFO, "Gatn: attach networking");
							restore();
						} else if(strcmp(oi.iname, I_FS) == 0) {
							mpi_log->write(LMT_INFO, "Gatn: attach FS");
							restore();
						}
					} else if(pn->flags & (NF_UNINIT | NF_REMOVE)) { // release
						if(strcmp(oi.iname, I_NET) == 0) {
							mpi_log->write(LMT_INFO, "Gatn: detach networking");
							stop(true);
						} else if(strcmp(oi.iname, I_FS) == 0) {
							mpi_log->write(LMT_INFO, "Gatn: detach FS");
							stop(true);
						}
					}
					if(pn->flags & NF_UNLOAD) {
						if(strcmp(oi.iname, I_GATN_EXTENSION) == 0) {
							// unregister extension
							mpi_log->fwrite(LMT_INFO, "Gatn: unregister class '%s'", oi.cname);
							enum_servers([](_server_t *psrv, void *udata) {
								_object_info_t *poi = (_object_info_t *)udata;
								server *ps = (server *)psrv;

								ps->release_class(poi->cname);
							}, &oi);

						}
					}
				}
			} break;
		}

		return r;
	}

	bool configure(_cstr_t json_fname) {
		bool r = false;

		mpi_json = dynamic_cast<iJSON *>(_gpi_repo_->object_by_iname(I_JSON, RF_ORIGINAL));

		if(mpi_json) {
			HTCONTEXT jcxt = mpi_json->create_context();

			if(jcxt) {
				iFS *pi_fs = dynamic_cast<iFS *>(_gpi_repo_->object_by_iname(I_FS, RF_ORIGINAL));

				if(pi_fs) {
					iFileIO *pi_fio = pi_fs->open(json_fname, O_RDONLY);

					if(pi_fio) {
						_cstr_t ptr = (_cstr_t)pi_fio->map(MPF_READ);

						if(ptr) {
							if(mpi_json->parse(jcxt, ptr, pi_fio->size())) {
								load_extensions(jcxt);
								configure_servers(jcxt);
							} else
								mpi_log->fwrite(LMT_ERROR, "Gatn: Failed to parse configuration file '%s'", json_fname);
						} else
							mpi_log->fwrite(LMT_ERROR, "Unable to map content of '%s'", json_fname);

						pi_fs->close(pi_fio);
					} else
						mpi_log->fwrite(LMT_ERROR, "Gatn: Unable to open file '%s'", json_fname);

					_gpi_repo_->object_release(pi_fs);
				} else
					mpi_log->write(LMT_ERROR, "Gatn: Unable to obtaian FS interface");

				mpi_json->destroy_context(jcxt);
			} else
				mpi_log->write(LMT_ERROR, "Gatn: Unable to create context for JSON parser");

			_gpi_repo_->object_release(mpi_json);
		} else
			mpi_log->write(LMT_ERROR, "Gatn: Unable to obtain JSON interface");

		return r;
	}

	bool reload(_cstr_t server_name=NULL) {
		bool r = false;

		//...

		return r;
	}

	_server_t *create_server(_cstr_t name, _u32 port,
				_cstr_t doc_root,
				_cstr_t cache_path,
				_cstr_t no_cache="", // non cacheable area inside documents root (by example: folder1:folder2:...)
				_cstr_t path_disable="", // disabled area inside documents root (by example: /folder1/:/folder2/:...)
				_u32 buffer_size=SERVER_BUFFER_SIZE,
				_u32 max_workers=HTTP_MAX_WORKERS,
				_u32 max_connections=HTTP_MAX_CONNECTIONS,
				_u32 connection_timeout=HTTP_CONNECTION_TIMEOUT,
				SSL_CTX *ssl_context=NULL
				) {
		_server_t *r = NULL;
		_u32 sz = 0;

		if(mpi_map) {
			if(!(r = (_server_t *)mpi_map->get(name, strlen(name), &sz))) {
				server srv;
				server *psrv = (server *)mpi_map->add(name, strlen(name), &srv, sizeof(server));

				if(psrv) {
					if(psrv->init(name, port, doc_root, cache_path, no_cache,
							path_disable, buffer_size, max_workers, max_connections,
							connection_timeout, ssl_context)) {
						psrv->start();
						r = psrv;
					}
				}
			}
		}

		return r;
	}

	_server_t *server_by_name(_cstr_t name) {
		_u32 sz = 0;
		_server_t *r = NULL;

		if(name)
			r = (_server_t *)mpi_map->get(name, strlen(name), &sz);

		return r;
	}

	void remove_server(_server_t *p_srv) {
		server *p = dynamic_cast<server *>(p_srv);

		if(p) {
			destroy(p);
			mpi_map->del(p->m_name, strlen(p->m_name));
		}
	}

	bool stop_server(_server_t *p_srv) {
		bool r = false;
		server *ps = dynamic_cast<server *>(p_srv);

		if(ps) {
			ps->stop();
			r = !ps->is_running();
		}

		return r;
	}

	bool start_server(_server_t *p_srv) {
		bool r = false;
		server *ps = dynamic_cast<server *>(p_srv);

		if(ps)
			r = ps->start();

		return r;
	}

	void enum_servers(void (*cb_enum)(_server_t *, void *), void *udata=NULL) {
		_u32 sz = 0;
		_map_enum_t se = mpi_map->enum_open();

		if(se) {
			_server_t *p_srv = reinterpret_cast<_server_t *>(mpi_map->enum_first(se, &sz));

			while(p_srv) {
				cb_enum(p_srv, udata);
				p_srv = reinterpret_cast<_server_t *>(mpi_map->enum_next(se, &sz));
			}

			mpi_map->enum_close(se);
		}
	}

	SSL_CTX *create_ssl_context(_cstr_t method, _cstr_t cert, _cstr_t key) {
		SSL_CTX *r = NULL;
		const SSL_METHOD *p_method = ssl_select_method(method);

		if(p_method) {
			if((r = ssl_create_context(p_method))) {
				if(ssl_load_cert(r, cert)) {
					if(!ssl_load_key(r, key)) {
						SSL_CTX_free(r);
						r = NULL;
					}
				} else {
					SSL_CTX_free(r);
					r = NULL;
				}
			}
		}

		if(!r)
			mpi_log->fwrite(LMT_ERROR, "Gatn: %s", ssl_error_string());


		return r;
	}

	void destroy_ssl_context(SSL_CTX *cxt) {
		SSL_CTX_free(cxt);
	}
};

static cGatn _g_gatn_;

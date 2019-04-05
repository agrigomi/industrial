#include <string.h>
#include "iHttpHost.h"
#include "iRepository.h"
#include "iNet.h"
#include "iLog.h"
#include "iArgs.h"

#define DEFAULT_HTTP_PORT	8080
#define DEFAULT_HTTPD_PREFIX	"httpd-"
#define MAX_SERVER_NAME		32
#define MAX_DOC_ROOT		256
#define SERVER_NAME		"ExtHttp (proholic)"
#define BUFFER_SIZE		(16*1024)

class cHttpHost: public iHttpHost {
private:
	iNet		*mpi_net;
	iFS		*mpi_fs;
	iFileCache	*mpi_fcache;
	iLog		*mpi_log;
	iArgs		*mpi_args;
	iMap		*mpi_map;
	_u32		httpd_index;

	typedef struct { // server record
		_char_t		name[MAX_SERVER_NAME];
		_u32		port;
		_char_t		doc_root[MAX_DOC_ROOT];
		iHttpServer	*pi_http_server;
		cHttpHost	*p_http_host;
	}_server_t;

	void set_handlers(_server_t *p_srv) {
		p_srv->pi_http_server->on_event(HTTP_ON_OPEN, [](iHttpConnection *pi_httpc, void *udata) {
			//...
		}, p_srv);

		p_srv->pi_http_server->on_event(HTTP_ON_REQUEST, [](iHttpConnection *pi_httpc, void *udata) {
			_server_t *p_srv = (_server_t *)udata;
			_u8 method = pi_httpc->req_method();
			_cstr_t url = pi_httpc->req_url();
			_char_t doc[1024]="";
			iLog *pi_log = p_srv->p_http_host->mpi_log;
			_char_t req_ip[32]="";

			pi_httpc->peer_ip(req_ip, sizeof(req_ip));

			if((url && strlen(url) + strlen(p_srv->doc_root) < sizeof(doc)-1)) {
				snprintf(doc, sizeof(doc), "%s%s",
					p_srv->doc_root,
					(strcmp(url, "/") == 0) ? "/index.html" : url);

				pi_httpc->res_var("Server", SERVER_NAME);
				pi_httpc->res_var("Allow", "GET, HEAD");

				if(method == HTTP_METHOD_GET) {
					if(p_srv->p_http_host->mpi_fcache) {
						HFCACHE fc = p_srv->p_http_host->mpi_fcache->open(doc);
						if(fc) { // open file cache
							_ulong sz = 0;

							pi_log->fwrite(LMT_INFO, "GET '%s' (%s)", pi_httpc->req_uri(), req_ip);
							_u8 *ptr = (_u8 *)p_srv->p_http_host->mpi_fcache->ptr(fc, &sz);
							if(ptr) {
								pi_httpc->res_content_len(sz);
								pi_httpc->res_code(HTTPRC_OK);
								pi_httpc->res_mtime(p_srv->p_http_host->mpi_fcache->mtime(fc));
								pi_httpc->res_write(ptr, sz);
							} else { // can't get pointer to file content
								pi_httpc->res_code(HTTPRC_INTERNAL_SERVER_ERROR);
								pi_httpc->res_content_len(pi_httpc->res_write("Internal server error"));
								pi_log->fwrite(LMT_ERROR, "%s: Internal error", p_srv->name);
							}
							p_srv->p_http_host->mpi_fcache->close(fc);
						} else {
							pi_httpc->res_code(HTTPRC_NOT_FOUND);
							pi_httpc->res_content_len(pi_httpc->res_write("Not Found"));
							pi_log->fwrite(LMT_ERROR, "%s: '%s' Not found (%s)", p_srv->name, pi_httpc->req_uri(), req_ip);
						}
					} else
						pi_httpc->res_code(HTTPRC_INTERNAL_SERVER_ERROR);
				} else if(method == HTTP_METHOD_HEAD) {
					if(p_srv->p_http_host->mpi_fcache) {
						HFCACHE fc = p_srv->p_http_host->mpi_fcache->open(doc);
						if(fc) { // open file cache
							//pi_log->fwrite(LMT_INFO, "HEAD '%s' (%s)", doc, req_ip);
							pi_httpc->res_code(HTTPRC_OK);
							pi_httpc->res_mtime(p_srv->p_http_host->mpi_fcache->mtime(fc));
							p_srv->p_http_host->mpi_fcache->close(fc);
						} else {
							_char_t req_ip[32]="";

							pi_httpc->peer_ip(req_ip, sizeof(req_ip));
							pi_httpc->res_code(HTTPRC_NOT_FOUND);
							pi_httpc->res_content_len(pi_httpc->res_write("Not Found"));
							pi_log->fwrite(LMT_ERROR, "%s: '%s' Not found (%s)", p_srv->name, pi_httpc->req_uri(), req_ip);
						}
					} else {
						pi_httpc->res_code(HTTPRC_INTERNAL_SERVER_ERROR);
						pi_httpc->res_content_len(pi_httpc->res_write("Internal server error"));
						pi_log->fwrite(LMT_ERROR, "%s: Internal error", p_srv->name);
					}
				} else {
					_char_t req_ip[32]="";

					pi_httpc->peer_ip(req_ip, sizeof(req_ip));
					pi_httpc->res_code(HTTPRC_METHOD_NOT_ALLOWED);
					pi_httpc->res_content_len(pi_httpc->res_write("Method not allowed"));
					pi_log->fwrite(LMT_ERROR, "%s: Method not allowed (%s)", p_srv->name, req_ip);
					pi_log->write(LMT_TEXT, pi_httpc->req_header());

					_u32 sz = 0;
					_str_t data = (_str_t)pi_httpc->req_data(&sz);
					if(data && sz) {
						fwrite(data, sz, 1, stdout);
						fwrite("\r\n", 2, 1, stdout);
					}
				}
			} else {
				pi_httpc->res_code(HTTPRC_BAD_REQUEST);
				pi_httpc->res_content_len(pi_httpc->res_write("Invalid request"));
				pi_log->fwrite(LMT_ERROR, "Invalid request URI (%s)", req_ip);
				if(url)
					pi_log->write(LMT_TEXT, url);
			}
		}, p_srv);

		p_srv->pi_http_server->on_event(HTTP_ON_REQUEST_DATA, [](iHttpConnection *pi_httpc, void *udata) {
			_server_t *p_srv = (_server_t *)udata;
			iLog *pi_log = p_srv->p_http_host->mpi_log;
			_u32 sz = 0;
			_str_t data = (_str_t)pi_httpc->req_data(&sz);

			pi_log->fwrite(LMT_INFO, ">>> on_request_data (%d)", sz);
			if(data) {
				fwrite(data, sz, 1, stdout);
				fwrite("\r\n", 2, 1, stdout);
			}
		}, p_srv);

		p_srv->pi_http_server->on_event(HTTP_ON_RESPONSE_DATA, [](iHttpConnection *pi_httpc, void *udata) {
			_server_t *p_srv = (_server_t *)udata;
			_ulong sz = 0;
			_char_t doc[1024]="";
			_cstr_t url = pi_httpc->req_url();
			_char_t req_ip[32]="";

			pi_httpc->peer_ip(req_ip, sizeof(req_ip));

			if((url && strlen(url) + strlen(p_srv->doc_root) < sizeof(doc)-1)) {
				snprintf(doc, sizeof(doc), "%s%s",
					p_srv->doc_root,
					(strcmp(url, "/") == 0) ? "/index.html" : url);

				if(p_srv->p_http_host->mpi_fcache) {
					HFCACHE fc = p_srv->p_http_host->mpi_fcache->open(doc);
					if(fc) { // open file cache
						_u8 *ptr = (_u8 *)p_srv->p_http_host->mpi_fcache->ptr(fc, &sz);

						if(ptr) {
							_u32 res_sent = pi_httpc->res_content_sent();
							_u32 res_remainder = pi_httpc->res_remainder();

							pi_httpc->res_write(ptr + res_sent, res_remainder);
						}

						p_srv->p_http_host->mpi_fcache->close(fc);
					}
				}
			}
		}, p_srv);

		p_srv->pi_http_server->on_event(HTTP_ON_ERROR, [](iHttpConnection *pi_httpc, void *udata) {
			_server_t *p_srv = (_server_t *)udata;
			iLog *pi_log = p_srv->p_http_host->mpi_log;
			_char_t req_ip[32]="";

			pi_httpc->peer_ip(req_ip, sizeof(req_ip));

			pi_log->fwrite(LMT_ERROR, "%s: Error(%d) (%s)", p_srv->name, pi_httpc->error_code(), req_ip);

			_cstr_t hdr = pi_httpc->req_header();

			if(hdr)
				pi_log->fwrite(LMT_TEXT, "%s: %s", p_srv->name, hdr);

			pi_httpc->res_code(pi_httpc->error_code());
			pi_httpc->res_var("Server", SERVER_NAME);
		}, p_srv);

		p_srv->pi_http_server->on_event(HTTP_ON_CLOSE, [](iHttpConnection *pi_httpc, void *udata) {
			//...
		}, p_srv);
	}

	void release_object(iRepository *pi_repo, iBase **pp_obj) {
		if(*pp_obj) {
			pi_repo->object_release(*pp_obj);
			*pp_obj = NULL;
		}
	}

	void create_first_server(void) {
		_str_t arg_port = mpi_args->value("httpd-port");
		_str_t arg_name = mpi_args->value("httpd-name");
		_str_t arg_root = mpi_args->value("httpd-root");
		_str_t arg_max_threads = mpi_args->value("httpd-max-threads");
		_str_t arg_max_conn = mpi_args->value("httpd-max-connections");
		_str_t arg_conn_timeout = mpi_args->value("httpd-connection-timeout");

		_char_t name[256]="";

		if(arg_port && arg_root) {
			if(arg_name)
				strncpy(name, arg_name, sizeof(name)-1);
			else
				snprintf(name, sizeof(name)-1, "%s%d", DEFAULT_HTTPD_PREFIX, httpd_index);

			_u32 max_threads = (arg_max_threads) ? atoi(arg_max_threads) : 16;
			_u32 max_conn = (arg_max_conn) ? atoi(arg_max_conn) : 200;
			_u32 conn_timeout = (arg_conn_timeout) ? atoi(arg_conn_timeout) : 10;
			create(name, atoi(arg_port), arg_root, max_threads, max_conn, conn_timeout);
		}
	}

	void stop_host(void) {
		_map_enum_t me = mpi_map->enum_open();

		mpi_log->write(LMT_INFO, "Stop ExtHttp...");
		if(me) {
			_u32 sz = 0;
			_server_t *p_srv = (_server_t *)mpi_map->enum_first(me, &sz);

			while(p_srv) {
				if(p_srv->pi_http_server) {
					mpi_log->fwrite(LMT_INFO, "ExtHttp: Stop server '%s'", p_srv->name);
					_gpi_repo_->object_release(p_srv->pi_http_server);
					p_srv->pi_http_server = NULL;
				}
				p_srv = (_server_t *)mpi_map->enum_next(me, &sz);
			}

			mpi_map->enum_close(me);
		}
	}

public:
	BASE(cHttpHost, "cHttpHost", RF_ORIGINAL, 1,0,0);

	bool object_ctl(_u32 cmd, void *arg, ...) {
		bool r = false;

		switch(cmd) {
			case OCTL_INIT: {
				iRepository *pi_repo = (iRepository *)arg;

				mpi_net = NULL;
				mpi_fs = NULL;
				mpi_fcache = NULL;
				httpd_index = 1;

				mpi_log = (iLog *)pi_repo->object_by_iname(I_LOG, RF_ORIGINAL);
				mpi_map = (iMap *)pi_repo->object_by_iname(I_MAP, RF_CLONE);
				mpi_args = (iArgs *)pi_repo->object_by_iname(I_ARGS, RF_ORIGINAL);

				pi_repo->monitoring_add(NULL, I_NET, NULL, this, SCAN_ORIGINAL);
				pi_repo->monitoring_add(NULL, I_FS, NULL, this, SCAN_ORIGINAL);

				if(mpi_log && mpi_map)
					r = mpi_map->init(31);

				if(mpi_net && mpi_fs && mpi_fcache)
					create_first_server();
			} break;
			case OCTL_UNINIT: {
				iRepository *pi_repo = (iRepository *)arg;

				stop_host();
				release_object(pi_repo, (iBase **)&mpi_net);
				release_object(pi_repo, (iBase **)&mpi_fcache);
				release_object(pi_repo, (iBase **)&mpi_fs);
				release_object(pi_repo, (iBase **)&mpi_map);
				release_object(pi_repo, (iBase **)&mpi_log);
				release_object(pi_repo, (iBase **)&mpi_args);
				r = true;
			} break;
			case OCTL_NOTIFY: {
				_notification_t *pn = (_notification_t *)arg;
				_object_info_t oi;

				memset(&oi, 0, sizeof(_object_info_t));
				if(pn->object) {
					pn->object->object_info(&oi);

					if(pn->flags & NF_INIT) { // catch
						if(strcmp(oi.iname, I_NET) == 0) {
							mpi_log->write(LMT_INFO, "ExtHttp: catch networking");
							mpi_net = (iNet *)pn->object;
						} else if(strcmp(oi.iname, I_FS) == 0) {
							mpi_log->write(LMT_INFO, "ExtHttp: catch FS support");
							mpi_fs = (iFS *)pn->object;
							if((mpi_fcache = (iFileCache *)_gpi_repo_->object_by_iname(I_FILE_CACHE, RF_CLONE)))
								mpi_fcache->init("/tmp", "ExtHttp");
						}
					} else if(pn->flags & (NF_UNINIT | NF_REMOVE)) { // release
						if(strcmp(oi.iname, I_NET) == 0) {
							mpi_log->write(LMT_INFO, "ExtHttp: release networking");
							stop_host();
							release_object(_gpi_repo_, (iBase **)&mpi_net);
						} else if(strcmp(oi.iname, I_FS) == 0) {
							mpi_log->write(LMT_INFO, "ExtHttp: release FS support");
							stop_host();
							release_object(_gpi_repo_, (iBase **)&mpi_fcache);
							release_object(_gpi_repo_, (iBase **)&mpi_fs);
						}
					}
				}
			} break;
		}

		return r;
	}

	bool create(_cstr_t name, _u32 port, _cstr_t doc_root, _u32 max_threads=32,
				_u32 max_connections=200, _u32 connection_timeout=10) {
		bool r = false;
		_server_t srv;

		if(mpi_net) {
			_u32 sz=0;

			if(mpi_map->get(name, strlen(name), &sz))
				mpi_log->fwrite(LMT_ERROR, "HTTP: Server '%s' already exists", name);
			else {
				memset(&srv, 0, sizeof(srv));
				strncpy(srv.name, name, sizeof(srv.name)-1);
				strncpy(srv.doc_root, doc_root, sizeof(srv.doc_root)-1);
				srv.port = port;
				srv.p_http_host = this;

				if((srv.pi_http_server = mpi_net->create_http_server(port, BUFFER_SIZE,
								max_threads, max_connections, connection_timeout))) {
					_server_t *p_srv = (_server_t *)mpi_map->add(name, strlen(name), &srv, sizeof(srv));
					if(p_srv) {
						set_handlers(p_srv);
						httpd_index++;
						r = true;
					} else
						_gpi_repo_->object_release(srv.pi_http_server);
				} else
					mpi_log->fwrite(LMT_ERROR, "Unable to create server '%s' on port %d", name, port);
			}
		} else
			mpi_log->write(LMT_ERROR, "No network support");

		return r;
	}

	bool start(_cstr_t name) {
		bool r = false;
		_u32 sz = 0;
		_server_t *p_srv = (_server_t *)mpi_map->get(name, strlen(name), &sz);

		if(p_srv) {
			if(!p_srv->pi_http_server) {
				if((p_srv->pi_http_server = mpi_net->create_http_server(p_srv->port, BUFFER_SIZE))) {
					set_handlers(p_srv);
					r = true;
				}
			} else
				r = true;
		}

		return r;
	}

	bool stop(_cstr_t name) {
		bool r = false;
		_u32 sz = 0;
		_server_t *p_srv = (_server_t *)mpi_map->get(name, strlen(name), &sz);

		if(p_srv) {
			if(p_srv->pi_http_server) {
				_gpi_repo_->object_release(p_srv->pi_http_server);
				p_srv->pi_http_server = NULL;
			}
			r = true;
		}

		return r;
	}

	bool remove(_cstr_t name) {
		bool r = false;
		_u32 sz = 0;
		_server_t *p_srv = (_server_t *)mpi_map->get(name, strlen(name), &sz);

		if(p_srv) {
			stop(p_srv->name);
			mpi_map->del(name, strlen(name));
			r = true;
		}

		return r;
	}

	void enumerate(_enum_http_t *pcb, void *udata=NULL) {
		_map_enum_t me = mpi_map->enum_open();

		if(me) {
			_u32 sz = 0;
			_server_t *p_srv = (_server_t *)mpi_map->enum_first(me, &sz);

			while(p_srv) {
				pcb((p_srv->pi_http_server) ? true : false, p_srv->name, p_srv->port, p_srv->doc_root, udata);
				p_srv = (_server_t *)mpi_map->enum_next(me, &sz);
			}

			mpi_map->enum_close(me);
		}
	}
};

static cHttpHost _g_http_host_;

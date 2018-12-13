#include <string.h>
#include "iHttpHost.h"
#include "iRepository.h"
#include "iNet.h"
#include "iFS.h"
#include "iLog.h"
#include "iMemory.h"
#include "iArgs.h"

#define DEFAULT_HTTP_PORT	8080
#define DEFAULT_HTTPD_PREFIX	"httpd-"
#define MAX_SERVER_NAME		32
#define MAX_DOC_ROOT		256

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

			if((strlen(url) + strlen(p_srv->doc_root) < sizeof(doc)-1)) {
				snprintf(doc, sizeof(doc), "%s%s",
					p_srv->doc_root,
					(strcmp(url, "/") == 0) ? "/index.html" : url);

				if(method == HTTP_METHOD_GET) {
					if(p_srv->p_http_host->mpi_fcache) {
						HFCACHE fc = p_srv->p_http_host->mpi_fcache->open(doc);
						if(fc) { // open file cache
							_ulong sz = 0;

							_u8 *ptr = (_u8 *)p_srv->p_http_host->mpi_fcache->ptr(fc, &sz);
							if(ptr) {
								pi_httpc->res_content_len(sz);
								pi_httpc->res_code(HTTPRC_OK);
								pi_httpc->set_udata((_ulong)fc); // keep file cache
								pi_httpc->res_write(ptr, sz);
							} else { // can't get pointer to file content
								pi_httpc->res_code(HTTPRC_INTERNAL_SERVER_ERROR);
								pi_log->fwrite(LMT_ERROR, "%s: Internal error", p_srv->name);
							}
						} else {
							pi_httpc->res_code(HTTPRC_NOT_FOUND);
							pi_log->fwrite(LMT_ERROR, "%s: '%s' Not found", p_srv->name, doc);
						}
					} else
						pi_httpc->res_code(HTTPRC_INTERNAL_SERVER_ERROR);
				} else {
					pi_httpc->res_code(HTTPRC_METHOD_NOT_ALLOWED);
					pi_log->fwrite(LMT_ERROR, "%s: Method not allowed", p_srv->name);
				}
			} else
				pi_httpc->res_code(HTTPRC_REQ_URI_TOO_LARGE);
		}, p_srv);

		p_srv->pi_http_server->on_event(HTTP_ON_REQUEST_DATA, [](iHttpConnection *pi_httpc, void *udata) {
			//...
		}, p_srv);

		p_srv->pi_http_server->on_event(HTTP_ON_RESPONSE_DATA, [](iHttpConnection *pi_httpc, void *udata) {
			_server_t *p_srv = (_server_t *)udata;
			HFCACHE fc = (HFCACHE)pi_httpc->get_udata(); // restore file cache
			_ulong sz = 0;

			if(fc) {
				_u8 *ptr = (_u8 *)p_srv->p_http_host->mpi_fcache->ptr(fc, &sz);
				_u32 res_sent = pi_httpc->res_content_sent();
				_u32 res_remainder = pi_httpc->res_remainder();
				pi_httpc->res_write(ptr + res_sent, res_remainder);
			}
		}, p_srv);

		p_srv->pi_http_server->on_event(HTTP_ON_ERROR, [](iHttpConnection *pi_httpc, void *udata) {
			_server_t *p_srv = (_server_t *)udata;
			iLog *pi_log = p_srv->p_http_host->mpi_log;

			pi_log->fwrite(LMT_ERROR, "%s: Error(%d)", p_srv->name, pi_httpc->error_code());
		}, p_srv);

		p_srv->pi_http_server->on_event(HTTP_ON_CLOSE, [](iHttpConnection *pi_httpc, void *udata) {
			_server_t *p_srv = (_server_t *)udata;
			HFCACHE fc = (HFCACHE)pi_httpc->get_udata();

			if(fc) // close file cache
				p_srv->p_http_host->mpi_fcache->close(fc);
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
		_char_t name[256]="";

		if(arg_port && arg_root) {

			if(arg_name)
				strncpy(name, arg_name, sizeof(name)-1);
			else
				snprintf(name, sizeof(name)-1, "%s%d", DEFAULT_HTTPD_PREFIX, httpd_index);

			create(name, atoi(arg_port), arg_root);
		}
	}

	void stop_host(void) {
		//...
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
					r = true;

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
						if(strcmp(oi.iname, I_NET) == 0)
							mpi_net = (iNet *)pn->object;
						else if(strcmp(oi.iname, I_FS) == 0) {
							mpi_fs = (iFS *)pn->object;
							if((mpi_fcache = (iFileCache *)_gpi_repo_->object_by_iname(I_FILE_CACHE, RF_CLONE)))
								mpi_fcache->init("/tmp");
						}
					} else if(pn->flags & (NF_UNINIT | NF_REMOVE)) { // release
						if(strcmp(oi.iname, I_NET) == 0) {
							stop_host();
							release_object(_gpi_repo_, (iBase **)&mpi_net);
						} else if(strcmp(oi.iname, I_FS) == 0) {
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

	bool create(_cstr_t name, _u32 port, _cstr_t doc_root) {
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

				if((srv.pi_http_server = mpi_net->create_http_server(port))) {
					_server_t *p_srv = (_server_t *)mpi_map->add(name, strlen(name), &srv, sizeof(srv));
					if(p_srv) {
						set_handlers(p_srv);
						httpd_index++;
						r = true;
					} else
						_gpi_repo_->object_release(srv.pi_http_server);
				}
			}
		}

		return r;
	}

	bool start(_cstr_t name) {
		bool r = false;

		//...

		return r;
	}

	bool stop(_cstr_t name) {
		bool r = false;

		//...

		return r;
	}

	bool remove(_cstr_t name) {
		bool r = false;

		//...

		return r;
	}
};

static cHttpHost _g_http_host_;

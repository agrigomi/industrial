#include <string.h>
#include "iCmd.h"
#include "iGatn.h"
#include "iRepository.h"
#include "private.h"

// gatn actions
#define ACT_CREATE 	"create"
#define ACT_REMOVE	"remove"
#define ACT_START	"start"
#define ACT_STOP	"stop"
#define ACT_HOST	"host"
#define ACT_SERVER	"server"
#define ACT_LIST	"list"
#define ACT_LOAD	"load"
#define ACT_RELOAD	"reload"
#define ACT_ATTACH	"attach"
#define ACT_DETACH	"detach"

#define OPT_SERVER	"server"
#define OPT_HOST	"host"
#define OPT_NAME	"name"
#define OPT_PORT	"port"
#define OPT_ROOT	"root"
#define OPT_BUFFER	"buffer"
#define OPT_THREADS	"threads"
#define OPT_CONNECTIONS	"connections"
#define OPT_TIMEOUT	"timeout"
#define OPT_CACHE_PATH	"cache-path"
#define OPT_CACHE_KEY	"cache-key"
#define OPT_NOCACHE	"cache-exclude"
#define OPT_DISABLE	"root-exclude"
#define OPT_OPTIONS	"options"
#define OPT_SSL_METHOD	"ssl-method"
#define OPT_SSL_CERT	"ssl-cert"
#define OPT_SSL_KEY	"ssl-key"

static iGatn *gpi_gatn = NULL;

typedef struct {
	_cstr_t	a_name;
	_cmd_handler_t	*p_handler;
}_cmd_action_t;

static iGatn *get_gatn(void) {
	if(!gpi_gatn)
		gpi_gatn = (iGatn *)_gpi_repo_->object_by_iname(I_GATN, RF_ORIGINAL);

	return gpi_gatn;
}

static void fout(iIO *pi_io, _cstr_t fmt, ...) {
	if(pi_io) {
		va_list va;
		_char_t lb[2048]="";
		_u32 sz=0;

		va_start(va, fmt);
		sz = vsnprintf(lb, sizeof(lb), fmt, va);
		pi_io->write(lb, sz);
		va_end(va);
	}
}

static void gatn_create_handler(iCmd *pi_cmd, // interface to command object
			iCmdHost *pi_cmd_host, // interface to command host
			iIO *pi_io, // interface to I/O object
			_cmd_opt_t *p_opt, // options array
			_u32 argc, // number of arguments
			_cstr_t argv[] // arguments
			) {
	iGatn *pi_gatn = get_gatn();

	if(pi_gatn) {
		_cstr_t server = pi_cmd_host->option_value(OPT_SERVER, p_opt);
		_cstr_t name = pi_cmd_host->option_value(OPT_NAME, p_opt);
		_cstr_t port = pi_cmd_host->option_value(OPT_PORT, p_opt);
		_cstr_t root = pi_cmd_host->option_value(OPT_ROOT, p_opt);
		_cstr_t buffer = pi_cmd_host->option_value(OPT_BUFFER, p_opt);
		_cstr_t cache_path = pi_cmd_host->option_value(OPT_CACHE_PATH, p_opt);
		_cstr_t cache_key = pi_cmd_host->option_value(OPT_CACHE_KEY, p_opt);
		_cstr_t no_cache = pi_cmd_host->option_value(OPT_NOCACHE, p_opt);
		_cstr_t disable = pi_cmd_host->option_value(OPT_DISABLE, p_opt);
		_cstr_t threads = pi_cmd_host->option_value(OPT_THREADS, p_opt);
		_cstr_t connections = pi_cmd_host->option_value(OPT_CONNECTIONS, p_opt);
		_cstr_t timeout = pi_cmd_host->option_value(OPT_TIMEOUT, p_opt);
		_cstr_t ssl_method_str = pi_cmd_host->option_value(OPT_SSL_METHOD, p_opt);
		_cstr_t ssl_cert_str = pi_cmd_host->option_value(OPT_SSL_CERT, p_opt);
		_cstr_t ssl_key_str = pi_cmd_host->option_value(OPT_SSL_KEY, p_opt);
		_cstr_t arg2 = pi_cmd_host->argument(argc, argv, p_opt, 2);
		_cstr_t arg3 = pi_cmd_host->argument(argc, argv, p_opt, 3);

		if(!arg2)
			fout(pi_io, "Usage: gatn create <server | host>\n");
		else {
			if(strcmp(arg2, ACT_SERVER) == 0) {
				// create server
				_cstr_t server_name = (name) ? name : arg3;

				if(server_name) {
					if(port && root) {
						SSL_CTX *ssl_cxt = NULL;

						if(ssl_method_str && ssl_cert_str && ssl_key_str) {
							const SSL_METHOD *method = ssl_select_method(ssl_method_str);

							if(method) {
								if((ssl_cxt = ssl_create_context(method))) {
									if(ssl_load_cert(ssl_cxt, ssl_cert_str)) {
										if(!ssl_load_key(ssl_cxt, ssl_key_str))
											fout(pi_io, "%s\n", ssl_error_string());
									} else
										fout(pi_io, "%s\n", ssl_error_string());
								} else
									fout(pi_io, "%s\n", ssl_error_string());
							} else
								fout(pi_io, "%s\n", ssl_error_string());
						}

						pi_gatn->create_server(server_name, atoi(port), root, cache_path,
									(no_cache) ? no_cache : "",
									(disable) ? disable : "",
									(buffer && atoi(buffer) > 0) ? atoi(buffer) * 1024 : SERVER_BUFFER_SIZE,
									(threads) ? atoi(threads) : HTTP_MAX_WORKERS,
									(connections) ? atoi(connections) : HTTP_MAX_CONNECTIONS,
									(timeout) ? atoi(timeout) : HTTP_CONNECTION_TIMEOUT,
									ssl_cxt);
					} else {
						fout(pi_io, "Mandatory parameter is missing\n");
						fout(pi_io, "Usage: gatn create server <server name> | --name=<server name> --port=... --root=...\
 [--cache_path=... --threads=... --connections=... --timeout=... --cache-exclude=...]\n");
					}
				} else
					fout(pi_io, "Server name missing\n");
			} else if(strcmp(arg2, ACT_HOST) == 0) {
				// create host
				_cstr_t host_name = (name) ? name : arg3;

				if(host_name) {
					if(root && server) {
						_server_t *p_srv = pi_gatn->server_by_name(server);

						if(p_srv) {
							if(p_srv->add_virtual_host(host_name, root,
									(cache_path) ? cache_path : "/tmp",
									(cache_key) ? cache_key : host_name,
									no_cache, disable))
								p_srv->start_virtual_host(host_name);
							else
								fout(pi_io, "Unable to create virtual host '%s'\n", host_name);
						} else
							fout(pi_io, "Wrong server name '%s'\n", server);
					} else {
						fout(pi_io, "Mandatory parameter missing\n");
						fout(pi_io, "Usage: gatn create host <host name> | --name=<host name> --root=... --server=... \
 [--cache_path=... --cache-key=... --cache-exclude=...]\n");
					}
				} else
					fout(pi_io, "Host name missing\n");
			} else
				fout(pi_io, "'%s' is unknown\n", arg2);
		}
	}
}

static void gatn_remove_handler(iCmd *pi_cmd, // interface to command object
			iCmdHost *pi_cmd_host, // interface to command host
			iIO *pi_io, // interface to I/O object
			_cmd_opt_t *p_opt, // options array
			_u32 argc, // number of arguments
			_cstr_t argv[] // arguments
			) {
	iGatn *pi_gatn = get_gatn();

	if(pi_gatn) {
		_cstr_t server = pi_cmd_host->option_value(OPT_SERVER, p_opt);
		_cstr_t name = pi_cmd_host->option_value(OPT_NAME, p_opt);
		_cstr_t arg2 = pi_cmd_host->argument(argc, argv, p_opt, 2); // server or host
		_cstr_t arg3 = pi_cmd_host->argument(argc, argv, p_opt, 3); // server name or host name

		if(!arg2 )
			fout(pi_io, "Usage: gatn remove <server | host>\n");
		else {
			if(strcmp(arg2, ACT_SERVER) == 0) {
				_cstr_t server_name = (name) ? name : arg3;

				if(server_name) {
					_server_t *p_srv = pi_gatn->server_by_name(server_name);

					if(p_srv)
						pi_gatn->remove_server(p_srv);
					else
						fout(pi_io, "Unamble to find server '%s'\n", server_name);
				} else
					fout(pi_io, "Usage: gatn remove server <server name> | --name=<server name>\n");
			} else if(strcmp(arg2, ACT_HOST) == 0) {
				_cstr_t host_name = (name) ? name : arg3;

				if(host_name && server) {
					_server_t *p_srv = pi_gatn->server_by_name(server);

					if(p_srv) {
						if(!p_srv->remove_virtual_host(host_name))
							fout(pi_io, "Unable to remove virtual host '%s'\n", host_name);
					} else
						fout(pi_io, "Unable to find server '%s'\n", server);
				} else
					fout(pi_io, "Usage: gatn remove host <name> | --name=<host name> --server=...\n");
			} else
				fout(pi_io, "'%s' is unknown\n", arg2);
		}
	}
}

static void gatn_start_handler(iCmd *pi_cmd, // interface to command object
			iCmdHost *pi_cmd_host, // interface to command host
			iIO *pi_io, // interface to I/O object
			_cmd_opt_t *p_opt, // options array
			_u32 argc, // number of arguments
			_cstr_t argv[] // arguments
			) {
	iGatn *pi_gatn = get_gatn();

	if(pi_gatn) {
		_cstr_t server = pi_cmd_host->option_value(OPT_SERVER, p_opt);
		_cstr_t name = pi_cmd_host->option_value(OPT_NAME, p_opt);
		_cstr_t arg2 = pi_cmd_host->argument(argc, argv, p_opt, 2); // server or host
		_cstr_t arg3 = pi_cmd_host->argument(argc, argv, p_opt, 3); // server name or host name

		if(!arg2)
			fout(pi_io, "Usage: gatn start <server | host>\n");
		else {
			if(strcmp(arg2, ACT_SERVER) == 0) {
				_cstr_t server_name = (name) ? name : arg3;

				if(server_name) {
					_server_t *p_srv = pi_gatn->server_by_name(server_name);

					if(p_srv)
						pi_gatn->start_server(p_srv);
					else
						fout(pi_io, "Unable to find server '%s'\n", server_name);
				} else
					fout(pi_io, "Usage: gatn start server <server name> | --name=<server name>\n");
			} else if(strcmp(arg2, ACT_HOST) == 0) {
				_cstr_t host_name = (name) ? name : arg3;

				if(host_name && server) {
					_server_t *p_srv = pi_gatn->server_by_name(server);

					if(p_srv) {
						if(!p_srv->start_virtual_host(host_name))
							fout(pi_io, "Unable to start virtual host '%s'\n", host_name);
					} else
						fout(pi_io, "Unable to find server '%s'\n", server);
				} else
					fout(pi_io, "Usage: gatn start host <name> | --name=<host name> --server=...\n");
			} else
				fout(pi_io, "'%s' is unknown\n", arg2);
		}
	}
}

static void gatn_stop_handler(iCmd *pi_cmd, // interface to command object
			iCmdHost *pi_cmd_host, // interface to command host
			iIO *pi_io, // interface to I/O object
			_cmd_opt_t *p_opt, // options array
			_u32 argc, // number of arguments
			_cstr_t argv[] // arguments
			) {
	iGatn *pi_gatn = get_gatn();

	if(pi_gatn) {
		_cstr_t server = pi_cmd_host->option_value(OPT_SERVER, p_opt);
		_cstr_t name = pi_cmd_host->option_value(OPT_NAME, p_opt);
		_cstr_t arg2 = pi_cmd_host->argument(argc, argv, p_opt, 2); // server or host
		_cstr_t arg3 = pi_cmd_host->argument(argc, argv, p_opt, 3); // server name or host name

		if(!arg2)
			fout(pi_io, "Usage: gatn stop <server | host>\n");
		else {
			if(strcmp(arg2, ACT_SERVER) == 0) {
				_cstr_t server_name = (name) ? name : arg3;

				if(server_name) {
					_server_t *p_srv = pi_gatn->server_by_name(server_name);

					if(p_srv)
						pi_gatn->stop_server(p_srv);
					else
						fout(pi_io, "Unable to find server '%s'\n", server_name);
				} else
					fout(pi_io, "Usage: gatn stop server <server name> | --name=<server name>\n");
			} else if(strcmp(arg2, ACT_HOST) == 0) {
				_cstr_t host_name = (name) ? name : arg3;

				if(host_name && server) {
					_server_t *p_srv = pi_gatn->server_by_name(server);

					if(p_srv) {
						if(!p_srv->stop_virtual_host(host_name))
							fout(pi_io, "Unable to stop virtual host '%s'\n", host_name);
					} else
						fout(pi_io, "Unable to find server '%s'\n", server);
				} else
					fout(pi_io, "Usage: gatn stop host <name> | --name=<host name> --server=<server name>\n");
			} else
				fout(pi_io, "'%s' is unknown\n", arg2);
		}
	}
}

static void gatn_list_handler(iCmd *pi_cmd, // interface to command object
			iCmdHost *pi_cmd_host, // interface to command host
			iIO *pi_io, // interface to I/O object
			_cmd_opt_t *p_opt, // options array
			_u32 argc, // number of arguments
			_cstr_t argv[] // arguments
			) {
	iGatn *pi_gatn = get_gatn();

	if(pi_gatn) {
		pi_gatn->enum_servers([](_server_t *srv, void *udata) {
			iIO *pi_io = (iIO *)udata;
			server *psrv = dynamic_cast<server *>(srv);

			if(psrv) {
				fout(pi_io, "[%c] %d %s %s\n", (psrv->is_running()) ? 'R' : 'S',
					psrv->port(), psrv->name(), psrv->host.get_root()->get_doc_root());

				psrv->enum_virtual_hosts([](_vhost_t *pvhost, void *udata) {
					iIO *pi_io = (iIO *)udata;

					fout(pi_io, "\t[%c] %s %s\n", (pvhost->is_running()) ? 'R' : 'S',
						pvhost->name(),
						pvhost->get_root()->get_doc_root());
				}, pi_io);
			}
		}, pi_io);
	}
}

static void gatn_load_handler(iCmd *pi_cmd, // interface to command object
			iCmdHost *pi_cmd_host, // interface to command host
			iIO *pi_io, // interface to I/O object
			_cmd_opt_t *p_opt, // options array
			_u32 argc, // number of arguments
			_cstr_t argv[] // arguments
			) {
	iGatn *pi_gatn = get_gatn();

	if(pi_gatn) {
		_cstr_t arg2 = pi_cmd_host->argument(argc, argv, p_opt, 2); // server or host

		if(arg2)
			pi_gatn->configure(arg2);
	}
}

static void gatn_reload_handler(iCmd *pi_cmd, // interface to command object
			iCmdHost *pi_cmd_host, // interface to command host
			iIO *pi_io, // interface to I/O object
			_cmd_opt_t *p_opt, // options array
			_u32 argc, // number of arguments
			_cstr_t argv[] // arguments
			) {
	//...
}

static void gatn_attach_handler(iCmd *pi_cmd, // interface to command object
			iCmdHost *pi_cmd_host, // interface to command host
			iIO *pi_io, // interface to I/O object
			_cmd_opt_t *p_opt, // options array
			_u32 argc, // number of arguments
			_cstr_t argv[] // arguments
			) {
	iGatn *pi_gatn = get_gatn();

	if(pi_gatn) {
		_cstr_t server = pi_cmd_host->option_value(OPT_SERVER, p_opt);
		_cstr_t host = pi_cmd_host->option_value(OPT_HOST, p_opt);
		_cstr_t name = pi_cmd_host->option_value(OPT_NAME, p_opt);
		_cstr_t options = pi_cmd_host->option_value(OPT_OPTIONS, p_opt);
		_u32 narg = (name) ? 1 : 2;
		_cstr_t class_name = (name) ? name : pi_cmd_host->argument(argc, argv, p_opt, narg);

		if(class_name) {
			_server_t *psrv = pi_gatn->server_by_name(server);

			if(psrv) {
				while(class_name) {
					psrv->attach_class(class_name, options, host);
					narg++;
					class_name = pi_cmd_host->argument(argc, argv, p_opt, narg);
				}
			} else
				fout(pi_io, "Unable to find server '%s'\n", server);
		} else
			fout(pi_io, "Usage: gatn attach <class name ...> | --name=<class name> --server=<server name> [--host=<host name> --options='...']\n");
	}
}

static void gatn_detach_handler(iCmd *pi_cmd, // interface to command object
			iCmdHost *pi_cmd_host, // interface to command host
			iIO *pi_io, // interface to I/O object
			_cmd_opt_t *p_opt, // options array
			_u32 argc, // number of arguments
			_cstr_t argv[] // arguments
			) {
	iGatn *pi_gatn = get_gatn();

	if(pi_gatn) {
		_cstr_t server = pi_cmd_host->option_value(OPT_SERVER, p_opt);
		_cstr_t host = pi_cmd_host->option_value(OPT_HOST, p_opt);
		_cstr_t name = pi_cmd_host->option_value(OPT_NAME, p_opt);
		_u32 narg = (name) ? 1 : 2;
		_cstr_t class_name = (name) ? name : pi_cmd_host->argument(argc, argv, p_opt, narg);

		if(class_name) {
			_server_t *psrv = pi_gatn->server_by_name(server);

			if(psrv) {
				while(class_name) {
					psrv->detach_class(class_name, host);
					narg++;
					class_name = pi_cmd_host->argument(argc, argv, p_opt, narg);
				}
			} else
				fout(pi_io, "Unable to find server '%s'\n", server);
		} else
			fout(pi_io, "Usage: gatn detach <class name ...> | --name=<class name> --server=<server name> [--host=<host name>]\n");
	}
}

static _cmd_action_t _g_gatn_actions_[] = {
	{ ACT_CREATE,	gatn_create_handler},
	{ ACT_REMOVE,	gatn_remove_handler},
	{ ACT_START,	gatn_start_handler},
	{ ACT_STOP,	gatn_stop_handler},
	{ ACT_LIST,	gatn_list_handler},
	{ ACT_LOAD,	gatn_load_handler},
	{ ACT_RELOAD,	gatn_reload_handler},
	{ ACT_ATTACH,	gatn_attach_handler},
	{ ACT_DETACH,	gatn_detach_handler},
	{ 0,		0}
};

static void gatn_handler(iCmd *pi_cmd, // interface to command object
			iCmdHost *pi_cmd_host, // interface to command host
			iIO *pi_io, // interface to I/O object
			_cmd_opt_t *p_opt, // options array
			_u32 argc, // number of arguments
			_cstr_t argv[] // arguments
			) {
	_cstr_t arg = pi_cmd_host->argument(argc, argv, p_opt, 1);

	if(arg) {
		_u32 n = 0;

		while(_g_gatn_actions_[n].a_name) {
			if(strcmp(arg, _g_gatn_actions_[n].a_name) == 0) {
				// call action handler
				_g_gatn_actions_[n].p_handler(pi_cmd, pi_cmd_host, pi_io,
								p_opt, argc, argv);
				break;
			}

			n++;
		}

		if(_g_gatn_actions_[n].a_name == 0)
			fout(pi_io, "Unknown gatn command '%s'\n", arg);
	}
}

static _cmd_opt_t _g_opt_[] = {
	{ OPT_SERVER,		OF_LONG|OF_VALUE,		0,		"Server name (--server=<name>)"},
	{ OPT_HOST,		OF_LONG|OF_VALUE,		0,		"Host name (--host=<name>)"},
	{ OPT_NAME,		OF_LONG|OF_VALUE,		0,		"Server or host name (--name=<name>)"},
	{ OPT_PORT,		OF_LONG|OF_VALUE,		0,		"Listen port number (--port=<number>)"},
	{ OPT_ROOT,		OF_LONG|OF_VALUE,		0,		"Path to documents root (--root=<path>)"},
	{ OPT_BUFFER,		OF_LONG|OF_VALUE,		0,		"Buffer size in Kb"},
	{ OPT_THREADS,		OF_LONG|OF_VALUE,		0,		"Max. number of HTTP worker threads (--threads=<num>)"},
	{ OPT_CONNECTIONS,	OF_LONG|OF_VALUE,		0,		"Max. number of HTTP connections (--connections=<num>)"},
	{ OPT_TIMEOUT,		OF_LONG|OF_VALUE,		0,		"Connection timeout in seconds (--timeout=<sec>)"},
	{ OPT_CACHE_PATH,	OF_LONG|OF_VALUE|OF_PRESENT,	(_str_t)"/tmp",	"Path to cache folder (/tmp by default)"},
	{ OPT_CACHE_KEY,	OF_LONG|OF_VALUE,		0,		"Cache key (folder name)"},
	{ OPT_NOCACHE,		OF_LONG|OF_VALUE,		0,		"Disable caching for spec. folders (--cache-exclude=/fldr1/:/fldr2/)"},
	{ OPT_DISABLE,		OF_LONG|OF_VALUE,		0,		"Disable folders inside documents root (--root-exclude=/folder1/:/folder2/)"},
	{ OPT_OPTIONS,		OF_LONG|OF_VALUE,		0,		"Class arguments (--options='...')"},
	{ OPT_SSL_METHOD,	OF_LONG|OF_VALUE,		0,		"SSL method (--ssl-method=TLSv1 | SSLv23 | TLSv1_1 | TLSv1_2 | DTLSv1)"},
	{ OPT_SSL_CERT,		OF_LONG|OF_VALUE,		0,		"SSL sertificate file (--ssl-cert=<file name>.pem"},
	{ OPT_SSL_KEY,		OF_LONG|OF_VALUE,		0,		"SSL primary key file (--ssl-key=<file name>.pem"},
	{ 0,			0,				0,		0 } // terminate options list
};

static _cmd_t _g_cmd_[] = {
	{ "gatn",	_g_opt_,	gatn_handler,	"Gatn management",
		"Manage Gatn servers and hosts:\n"
		ACT_CREATE "\t:Create server or host (gatn create server|host <options> [--server=<server name>])\n"
		ACT_REMOVE "\t:Remove server or host (gatn remove server|host <name> [--server=<server name>])\n"
		ACT_START  "\t:Start server or host (gatn start server|host <name> [--server=<server name>])\n"
		ACT_STOP   "\t:Stop server or host (gatn stop server|host <name> [--server=<server name>])\n"
		ACT_LIST   "\t:List servers\n"
		ACT_LOAD   "\t:Configure Gatn by JSON file (gatn load <JSON file name>)\n"
		ACT_RELOAD "\t:Reload configuration for server or host (gatn reload [server|host])\n"
		ACT_ATTACH "\t:Attach class (gatn attach <class name ...> | --name=<class name> --server=... [--host=... --options='...'])\n"
		ACT_DETACH "\t:Detach class (gatn detach <class name ...> | --name=<class name> --server=... [--host=...])\n\n"
		"Usage: gatn <command> [options] [arguments]\n"
	},
	{ 0,	0,	0,	0,	0,	0} // terminate command list
};

class cGatnCmd: public iCmd {
public:
	BASE(cGatnCmd, "cGatnCmd", RF_ORIGINAL, 1,0,0);

	bool object_ctl(_u32 c, void *arg, ...) {
		bool r = false;

		switch(c) {
			case OCTL_INIT:
				r = true;
				break;
			case OCTL_UNINIT:
				if(gpi_gatn) {
					_gpi_repo_->object_release(gpi_gatn);
					gpi_gatn = NULL;
				}
				r = true;
		}
		return r;
	}

	_cmd_t *get_info(void) {
		return _g_cmd_;
	}
};

static cGatnCmd _g_gatn_cmd_;

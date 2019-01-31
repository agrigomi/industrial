#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "iCmd.h"
#include "iHttpHost.h"
#include "iRepository.h"

#define ACT_CREATE	"create"
#define ACT_REMOVE	"remove"
#define ACT_START	"start"
#define ACT_STOP	"stop"
#define ACT_LIST	"list"
#define ACT_STATUS	"status"

#define OPT_HTTPD_NAME	"name"
#define OPT_HTTPD_PORT	"port"
#define OPT_HTTPD_ROOT	"root"

typedef struct {
	_cstr_t	a_name;
	_cmd_handler_t	*a_handler;
}_cmd_action_t;

static iHttpHost *gpi_http_host = NULL;

static iHttpHost *get_httpd(void) {
	if(!gpi_http_host) {
		gpi_http_host = (iHttpHost *)_gpi_repo_->object_by_iname(I_HTTP_HOST, RF_ORIGINAL);
	}

	return gpi_http_host;
}

static void fout(iIO *pi_io, _cstr_t fmt, ...) {
	if(pi_io) {
		va_list va;
		_char_t lb[1024]="";
		_u32 sz=0;

		va_start(va, fmt);
		sz = vsnprintf(lb, sizeof(lb), fmt, va);
		pi_io->write(lb, sz);
		va_end(va);
	}
}

static void cmd_httpd_create(iCmd *pi_cmd, // interface to command object
			iCmdHost *pi_cmd_host, // interface to command host
			iIO *pi_io, // interface to I/O object
			_cmd_opt_t *p_opt, // options array
			_u32 argc, // number of arguments
			_cstr_t argv[] // arguments
			) {
	iHttpHost *pi_httpd = get_httpd();

	if(pi_httpd) {
		_cstr_t name = pi_cmd_host->option_value(OPT_HTTPD_NAME, p_opt);
		_cstr_t port = pi_cmd_host->option_value(OPT_HTTPD_PORT, p_opt);
		_cstr_t root = pi_cmd_host->option_value(OPT_HTTPD_ROOT, p_opt);

		if(name && port && root) {
			if(!pi_httpd->create(name, atoi(port), root))
				fout(pi_io, "Failed to create http server '%s'\n", name);
		} else
			fout(pi_io, "usage: httpd create --name=... --port=... --root=...\n");
	}
}

static void cmd_httpd_remove(iCmd *pi_cmd, // interface to command object
			iCmdHost *pi_cmd_host, // interface to command host
			iIO *pi_io, // interface to I/O object
			_cmd_opt_t *p_opt, // options array
			_u32 argc, // number of arguments
			_cstr_t argv[] // arguments
			) {
	// server name should be in argument 2
	_cstr_t sname = pi_cmd_host->argument(argc, argv, p_opt, 2);
	iHttpHost *pi_http_host = get_httpd();

	if(sname && pi_http_host) {
		if(!pi_http_host->remove(sname))
			fout(pi_io, "Failed to remove server '%s'\n", sname);
	}
}

static void cmd_httpd_start(iCmd *pi_cmd, // interface to command object
			iCmdHost *pi_cmd_host, // interface to command host
			iIO *pi_io, // interface to I/O object
			_cmd_opt_t *p_opt, // options array
			_u32 argc, // number of arguments
			_cstr_t argv[] // arguments
			) {
	_cstr_t sname = pi_cmd_host->argument(argc, argv, p_opt, 2);
	iHttpHost *pi_http_host = get_httpd();

	if(sname && pi_http_host) {
		if(!pi_http_host->start(sname))
			fout(pi_io, "Failed to start server '%s'\n", sname);
	}
}

static void cmd_httpd_stop(iCmd *pi_cmd, // interface to command object
			iCmdHost *pi_cmd_host, // interface to command host
			iIO *pi_io, // interface to I/O object
			_cmd_opt_t *p_opt, // options array
			_u32 argc, // number of arguments
			_cstr_t argv[] // arguments
			) {
	_cstr_t sname = pi_cmd_host->argument(argc, argv, p_opt, 2);
	iHttpHost *pi_http_host = get_httpd();

	if(sname && pi_http_host) {
		if(!pi_http_host->stop(sname))
			fout(pi_io, "Failed to stop server '%s'\n", sname);
	}
}

static void cmd_httpd_list(iCmd *pi_cmd, // interface to command object
			iCmdHost *pi_cmd_host, // interface to command host
			iIO *pi_io, // interface to I/O object
			_cmd_opt_t *p_opt, // options array
			_u32 argc, // number of arguments
			_cstr_t argv[] // arguments
			) {
	iHttpHost *pi_http_host = get_httpd();

	pi_http_host->enumerate([](bool running, _cstr_t name, _u32 port, _cstr_t root, void *udata) {
		iIO *pi_io = (iIO *)udata;

		fout(pi_io, "[%c] %s\t\t%d\t\t%s\n", (running) ? 'R' : 'S', name, port, root);
	}, pi_io);
}

static _cmd_action_t _g_cmd_httpd_actions_[]={
	{ ACT_CREATE,		cmd_httpd_create },
	{ ACT_REMOVE,		cmd_httpd_remove },
	{ ACT_START,		cmd_httpd_start },
	{ ACT_STOP,		cmd_httpd_stop },
	{ ACT_LIST,		cmd_httpd_list },
	{ 0,			0 }
};

static void httpd_handler(iCmd *pi_cmd, // interface to command object
			iCmdHost *pi_cmd_host, // interface to command host
			iIO *pi_io, // interface to I/O object
			_cmd_opt_t *p_opt, // options array
			_u32 argc, // number of arguments
			_cstr_t argv[] // arguments
			) {
	_cstr_t arg = pi_cmd_host->argument(argc, argv, p_opt, 1);

	if(arg) {
		_u32 n = 0;

		while(_g_cmd_httpd_actions_[n].a_name) {
			if(strcmp(arg, _g_cmd_httpd_actions_[n].a_name) == 0) {
				// call action handler
				_g_cmd_httpd_actions_[n].a_handler(pi_cmd, pi_cmd_host, pi_io,
								p_opt, argc, argv);
				break;
			}

			n++;
		}

		if(_g_cmd_httpd_actions_[n].a_name == 0)
			fout(pi_io, "Unknown httpd command '%s'\n", arg);
	}
}

static _cmd_opt_t _g_opt[] = {
	{ OPT_HTTPD_PORT,	OF_LONG|OF_VALUE,	0,	"Listen port number"},
	{ OPT_HTTPD_NAME,	OF_LONG|OF_VALUE,	0,	"Name of http server" },
	{ OPT_HTTPD_ROOT,	OF_LONG|OF_VALUE,	0,	"Documents root directory" },
	{ 0,			0,			0,	0 } // terminate options list
};

static _cmd_t _g_cmd[] = {
	{ "httpd",	_g_opt,	httpd_handler,	"Http management",
		"Manage HTTP servers by following commands:\n"
		ACT_CREATE "\t\t:Create HTTP server ( httpd create --name=... --port=... --root=... )\n"
		ACT_LIST   "\t\t:List HTTP servers  ( httpd list )\n"
		ACT_REMOVE "\t\t:Remove HTTP server ( httpd remove <server name> )\n"
		ACT_START  "\t\t:Start HTTP server  ( httpd start <server name> )\n"
		ACT_STOP   "\t\t:Stop HTTP server   ( httpd stop <server name> )\n",
		"httpd <command> [options]"
	},
	{ 0,	0,	0,	0,	0,	0} // terminate command list
};

class cCmdHttpd: public iCmd {
public:
	BASE(cCmdHttpd, "cCmdHttpd", RF_ORIGINAL, 1,0,0);

	bool object_ctl(_u32 c, void *arg, ...) {
		bool r = false;

		switch(c) {
			case OCTL_INIT:
				r = true;
				break;
			case OCTL_UNINIT:
				if(gpi_http_host)
					_gpi_repo_->object_release(gpi_http_host);
				r = true;
		}
		return r;
	}

	_cmd_t *get_info(void) {
		return _g_cmd;
	}
};

static cCmdHttpd _g_cmd_httpd_;

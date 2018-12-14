#include <stdio.h>
#include <string.h>
#include "iCmd.h"
#include "iHttpHost.h"
#include "iRepository.h"

#define ACT_CREATE	"create"
#define ACT_REMOVE	"remove"
#define ACT_START	"start"
#define ACT_STOP	"stop"

typedef struct {
	_cstr_t	a_name;
	_cmd_handler_t	*a_handler;
}_cmd_action_t;


static iHttpHost *get_httpd(void) {
	static iHttpHost *gpi_http_host = NULL;

	if(!gpi_http_host)
		gpi_http_host = (iHttpHost *)_gpi_repo_->object_by_iname(I_HTTP_HOST, RF_ORIGINAL);

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

static void cmd_httpd_create(iCmd *pi_cmd, // interface to command opbect
			iCmdHost *pi_cmd_host, // interface to command host
			iIO *pi_io, // interface to I/O object
			_cmd_opt_t *p_opt, // options array
			_u32 argc, // number of arguments
			_str_t argv[] // arguments
			) {
}

static void cmd_httpd_remove(iCmd *pi_cmd, // interface to command opbect
			iCmdHost *pi_cmd_host, // interface to command host
			iIO *pi_io, // interface to I/O object
			_cmd_opt_t *p_opt, // options array
			_u32 argc, // number of arguments
			_str_t argv[] // arguments
			) {
}

static void cmd_httpd_start(iCmd *pi_cmd, // interface to command opbect
			iCmdHost *pi_cmd_host, // interface to command host
			iIO *pi_io, // interface to I/O object
			_cmd_opt_t *p_opt, // options array
			_u32 argc, // number of arguments
			_str_t argv[] // arguments
			) {
}

static void cmd_httpd_stop(iCmd *pi_cmd, // interface to command opbect
			iCmdHost *pi_cmd_host, // interface to command host
			iIO *pi_io, // interface to I/O object
			_cmd_opt_t *p_opt, // options array
			_u32 argc, // number of arguments
			_str_t argv[] // arguments
			) {
}

static _cmd_action_t _g_cmd_httpd_actions_[]={
	{ ACT_CREATE,		cmd_httpd_create },
	{ ACT_REMOVE,		cmd_httpd_remove },
	{ ACT_START,		cmd_httpd_start },
	{ ACT_STOP,		cmd_httpd_stop },
	{ 0,			0 }
};

static void httpd_handler(iCmd *pi_cmd, // interface to command opbect
			iCmdHost *pi_cmd_host, // interface to command host
			iIO *pi_io, // interface to I/O object
			_cmd_opt_t *p_opt, // options array
			_u32 argc, // number of arguments
			_str_t argv[] // arguments
			) {
	_str_t arg = pi_cmd_host->argument(argc, argv, p_opt, 1);

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
	{ "port",	OF_LONG,	0,	"Listen port number"},
	{ "name",	OF_LONG,	0,	"Name of http server" },
	{ "root",	OF_LONG,	0,	"Documents root directory" },
	{ 0,		0,		0,	0 } // terminate options list
};

static _cmd_t _g_cmd[] = {
	{ "httpd",	_g_opt,	httpd_handler,	"Http management",
		"Manage HTTP servers by following commands:\n"
		ACT_CREATE "\t\t:Create HTTP server ( httpd create --name=... --port=... --root=... )\n"
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
		return true;
	}

	_cmd_t *get_info(void) {
		return _g_cmd;
	}
};

static cCmdHttpd _g_cmd_httpd_;

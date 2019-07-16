#include "iCmd.h"
#include "iGatn.h"
#include "iRepository.h"

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

#define OPT_SERVER	"server"
#define OPT_NAME	"name"
#define OPT_PORT	"port"
#define OPT_ROOT	"root"
#define OPT_THREADS	"threads"
#define OPT_CONNECTIONS	"connections"
#define OPT_TIMEOUT	"timeout"
#define OPT_CACHE_PATH	"cache-path"
#define OPT_CACHE_KEY	"cache-key"
#define OPT_NOCACHE	"cache-exclude"

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
	//...
}

static void gatn_remove_handler(iCmd *pi_cmd, // interface to command object
			iCmdHost *pi_cmd_host, // interface to command host
			iIO *pi_io, // interface to I/O object
			_cmd_opt_t *p_opt, // options array
			_u32 argc, // number of arguments
			_cstr_t argv[] // arguments
			) {
	//...
}

static void gatn_start_handler(iCmd *pi_cmd, // interface to command object
			iCmdHost *pi_cmd_host, // interface to command host
			iIO *pi_io, // interface to I/O object
			_cmd_opt_t *p_opt, // options array
			_u32 argc, // number of arguments
			_cstr_t argv[] // arguments
			) {
	//...
}

static void gatn_stop_handler(iCmd *pi_cmd, // interface to command object
			iCmdHost *pi_cmd_host, // interface to command host
			iIO *pi_io, // interface to I/O object
			_cmd_opt_t *p_opt, // options array
			_u32 argc, // number of arguments
			_cstr_t argv[] // arguments
			) {
	//...
}

static void gatn_list_handler(iCmd *pi_cmd, // interface to command object
			iCmdHost *pi_cmd_host, // interface to command host
			iIO *pi_io, // interface to I/O object
			_cmd_opt_t *p_opt, // options array
			_u32 argc, // number of arguments
			_cstr_t argv[] // arguments
			) {
	//...
}

static void gatn_load_handler(iCmd *pi_cmd, // interface to command object
			iCmdHost *pi_cmd_host, // interface to command host
			iIO *pi_io, // interface to I/O object
			_cmd_opt_t *p_opt, // options array
			_u32 argc, // number of arguments
			_cstr_t argv[] // arguments
			) {
	//...
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

static _cmd_action_t _g_gatn_actions_[] = {
	{ ACT_CREATE,	gatn_create_handler},
	{ ACT_REMOVE,	gatn_remove_handler},
	{ ACT_START,	gatn_start_handler},
	{ ACT_STOP,	gatn_stop_handler},
	{ ACT_LIST,	gatn_list_handler},
	{ ACT_LOAD,	gatn_load_handler},
	{ ACT_RELOAD,	gatn_reload_handler},
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
	{ OPT_SERVER,		OF_LONG,			0,		"Server name (--server=<name>)"},
	{ OPT_NAME,		OF_LONG,			0,		"Server or host name (--name=<name>)"},
	{ OPT_PORT,		OF_LONG,			0,		"Listen port number (--port=<number>)"},
	{ OPT_ROOT,		OF_LONG,			0,		"Path to documents root (--root=<path>)"},
	{ OPT_THREADS,		OF_LONG,			0,		"Max. number of HTTP worker threads (--threads=<num>)"},
	{ OPT_CONNECTIONS,	OF_LONG,			0,		"Max number of HTTP connections (--connections=<num>)"},
	{ OPT_TIMEOUT,		OF_LONG,			0,		"Connection timeout in seconds (--timeout=<sec>)"},
	{ OPT_CACHE_PATH,	OF_LONG|OF_VALUE|OF_PRESENT,	(_str_t)"/tmp",	"Path to cache foldef (/tmp by default)"},
	{ OPT_CACHE_KEY,	OF_LONG,			0,		"Cache key (folder name)"},
	{ OPT_NOCACHE,		OF_LONG,			0,		"Disable caching for spec. folders (--cache-exclude=/fldr1:/fldr2)"},
	{ 0,			0,				0,		0 } // terminate options list
};

static _cmd_t _g_cmd_[] = {
	{ "gatn",	_g_opt_,	gatn_handler,	"Gatn management",
		"Manage Gatn servers and hosts:\n"
		ACT_CREATE "\t:Create server or host (gatn create <server|host> options)\n"
		ACT_REMOVE "\t:Remove server or host (gatn remove <server|host> name)\n"
		ACT_START  "\t:Start server or host (gatn start <server|host> name)\n"
		ACT_STOP   "\t:Stop server or host (gatn start <server|host> name)\n"
		ACT_LIST   "\t:List servers\n"
		ACT_LOAD   "\t:Configure Gatn by JSON file (gatn load <JSON file name>)\n"
		ACT_RELOAD "\t:Reload configuration for server or host (gatn reload [<server|host>])\n",
		"gatn <command> [options] [arguments]"
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
				if(gpi_gatn)
					_gpi_repo_->object_release(gpi_gatn);
				r = true;
		}
		return r;
	}

	_cmd_t *get_info(void) {
		return _g_cmd_;
	}
};

static cGatnCmd _g_gatn_cmd_;

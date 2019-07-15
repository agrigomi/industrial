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

static iGatn *get_gatn(void) {
	if(!gpi_gatn)
		gpi_gatn = (iGatn *)_gpi_repo_->object_by_iname(I_GATN, RF_ORIGINAL);

	return gpi_gatn;
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

static void gatn_handler(iCmd *pi_cmd, // interface to command object
			iCmdHost *pi_cmd_host, // interface to command host
			iIO *pi_io, // interface to I/O object
			_cmd_opt_t *p_opt, // options array
			_u32 argc, // number of arguments
			_cstr_t argv[] // arguments
			) {
}

static _cmd_opt_t _g_opt_[] = {
	{ 0,				0,				0,	0 } // terminate options list
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

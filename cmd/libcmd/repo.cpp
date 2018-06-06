#include <string.h>
#include <stdio.h>
#include "iCmd.h"

// options
#define OPT_EXT_ONLY	"ext-only"

// sctopns
#define ACT_LIST	"list"
#define ACT_LOAD	"load"
#define ACT_UNLOAD	"unload"

typedef struct {
	_cstr_t	a_name;
	_cmd_handler_t	*a_handler;
}_cmd_action_t;

class cCmdRepo;

static void fout(iIO *pi_io, _cstr_t fmt, ...) {
	va_list va;
	_char_t lb[1024]="";
	_u32 sz=0;

	va_start(va, fmt);
	sz = vsnprintf(lb, sizeof(lb), fmt, va);
	pi_io->write(lb, sz);
	va_end(va);
}

static void out(iIO *pi_io, _cstr_t str) {
	pi_io->write((void *)str, strlen(str));
}

static void cmd_repo_list(iCmd *pi_cmd, iCmdHost *pi_cmd_host,
			iIO *pi_io, _cmd_opt_t *p_opt,
			_u32 argc, _str_t argv[]) {
	//...
}

static void cmd_ext_load(iCmd *pi_cmd, iCmdHost *pi_cmd_host,
			iIO *pi_io, _cmd_opt_t *p_opt,
			_u32 argc, _str_t argv[]) {
	//...
}

static void cmd_ext_unload(iCmd *pi_cmd, iCmdHost *pi_cmd_host,
			iIO *pi_io, _cmd_opt_t *p_opt,
			_u32 argc, _str_t argv[]) {
	//...
}

static _cmd_action_t _g_cmd_repo_actions_[]={
	{ACT_LIST,		cmd_repo_list},
	{ACT_LOAD,		cmd_ext_load},
	{ACT_UNLOAD,		cmd_ext_unload},
	{0,			0}
};

static void cmd_repo_handler(iCmd *pi_cmd, iCmdHost *pi_cmd_host,
			iIO *pi_io, _cmd_opt_t *p_opt,
			_u32 argc, _str_t argv[]) {
	_str_t arg = pi_cmd_host->argument(argc, argv, p_opt, 1);

	if(arg) {
		_u32 n = 0;

		while(_g_cmd_repo_actions_[n].a_name) {
			if(strcmp(arg, _g_cmd_repo_actions_[n].a_name) == 0) {
				_g_cmd_repo_actions_[n].a_handler(pi_cmd, pi_cmd_host,
								pi_io, p_opt,
								argc, argv);
				break;
			}
			n++;
		}
	}
}

static _cmd_opt_t _g_cmd_repo_opt_[]={
	{OPT_EXT_ONLY,	OF_LONG,	0,	"Extensions only"},
	//...
	{0,		0,		0,	0}
};

static _cmd_t _g_cmd_repo_[]={
	{"repo",	_g_cmd_repo_opt_, cmd_repo_handler,
		"Repository management",
		"Manage reposiotory by following actions:\n"
		"\t" ACT_LIST "\t\t\tPrint available objects\n"
		"\t" ACT_LOAD "\t\t\tLoad extension\n"
		"\t" ACT_UNLOAD "\t\t\tUnload extension\n",
		"repo [options] <action>"
	},
	{0,	0,	0,	0,	0,	0}
};

class cCmdRepo: public iCmd {
public:
	bool object_ctl(_u32 cmd, void *arg, ...) {
		bool r = false;

		switch(cmd) {
			case OCTL_INIT:
				r = true;
				break;
			case OCTL_UNINIT:
				r = true;
				break;
		}

		return r;
	}

	_cmd_t *get_info(void) {
		return _g_cmd_repo_;
	}
};

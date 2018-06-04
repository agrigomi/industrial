#include "iCmd.h"

typedef struct {
	_cstr_t	a_name;
	_cmd_handler_t	*a_handler;
}_cmd_action_t;

class cCmdRepo;

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
	{"list",		cmd_repo_list},
	{"load",		cmd_ext_load},
	{"unload",		cmd_ext_unload},
	{0,			0}
};

static void cmd_repo_handler(iCmd *pi_cmd, iCmdHost *pi_cmd_host,
			iIO *pi_io, _cmd_opt_t *p_opt,
			_u32 argc, _str_t argv[]) {
	_str_t arg = pi_cmd_host->argument(argc, argv, p_opt, 1);

	if(arg) {
		_u32 n = 0;

		while(_g_cmd_repo_actions_[n].a_name) {
			_g_cmd_repo_actions_[n].a_handler(pi_cmd, pi_cmd_host, pi_io, p_opt,
								argc, argv);
			n++;
		}
	}
}

static _cmd_opt_t _g_cmd_repo_opt_[]={
	//...
	{0,	0,	0,	0}
};

static _cmd_t _g_cmd_repo_[]={
	{"repo",	_g_cmd_repo_opt_, cmd_repo_handler,
		"Repository management",
		"Manage reposiotory by following actions:\n"
		"\tlist		Print available objects\n"
		"\tload		Load extension\n"
		"\tunload	Unload extension\n",
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

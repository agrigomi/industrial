#include "iRepository.h"
#include "iCmd.h"

class cHelp;

void help_handler(iCmd *pi_cmd, iCmdHost *pi_cmd_host,
			iIO *pi_io, _cmd_opt_t *p_opt,
			_u32 argc, _str_t argv[]) {
	//...
}

static _cmd_opt_t _g_help_opt_[] = {
	{"p",	OF_VALUE,	0,	"Page mode. example:  -p <rows>"},
	{"f",	0,		0,	"Show full help"},
	{0,	0,		0,	0}
};

static _cmd_t _g_help_cmd_[] = {
	{"help",	_g_help_opt_,	help_handler,	"usage: help [options] [command]"},
	{0,		0,		0,		0}
};

class cHelp: public iCmd {
public:
	BASE(cHelp, "cHelp", RF_ORIGINAL, 1,0,0);

	bool object_ctl(_u32 cmd, void *arg, ...) {
		bool r = false;

		switch(cmd) {
			case OCTL_INIT: {
				r = true;
			} break;

			case OCTL_UNINIT: {
				r = true;
			} break;
		}

		return r;
	}

	_cmd_t *get_info(void) {
		return _g_help_cmd_;
	}
};

static cHelp _g_help_;

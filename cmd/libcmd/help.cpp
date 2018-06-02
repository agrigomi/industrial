#include "iRepository.h"
#include "iCmd.h"

class cHelp;

#define OPT_FULL	"f" // full help including usage and options information
#define OPT_PAGE	"p" // page mode (require number of rows to display)

void help_handler(iCmd *pi_cmd, iCmdHost *pi_cmd_host,
			iIO *pi_io, _cmd_opt_t *p_opt,
			_u32 argc, _str_t argv[]) {
	bool bfull = pi_cmd_host->option_check(OPT_FULL, p_opt);
	_str_t rows = pi_cmd_host->option_value(OPT_PAGE, p_opt);
	_str_t arg = pi_cmd_host->argument(argc, argv, p_opt, 1);

	asm("nop");
}

static _cmd_opt_t _g_help_opt_[] = {
	{OPT_PAGE,	OF_VALUE,	0,	"Page mode. example:  -p <rows>"},
	{OPT_FULL,	0,		0,	"Show full help"},
	{0,		0,		0,	0}
};

static _cmd_t _g_help_cmd_[] = {
	{"help",	_g_help_opt_,	help_handler,
		"Display help information", 		// summary
		0,					// description
		"usage: help [options] [command]"	// usage
	},

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

#include <stdlib.h>
#include <string.h>
#include "iRepository.h"
#include "iCmd.h"

class cCmdHelp;

#define OPT_FULL	"f" // full help including usage and options information

static void output(iIO *pi_io, _cstr_t txt) {
	if(pi_io)
		pi_io->write((void *)txt, strlen(txt));
}

static void help_command(_cmd_t *p_cmd, iIO *pi_io, bool bfull) {
	if(p_cmd->cmd_summary) {
		output(pi_io, p_cmd->cmd_name);
		output(pi_io, ": ");
		output(pi_io, p_cmd->cmd_summary);
		output(pi_io, "\n");
	}

	if(bfull) {
		if(p_cmd->cmd_description) {
			output(pi_io, "\n");
			output(pi_io, p_cmd->cmd_description);
			output(pi_io, "\n");
		}

		if(p_cmd->cmd_options) {
			_u32 n = 0;
			_cmd_opt_t *p_opt = p_cmd->cmd_options;

			output(pi_io, "Options: \n");
			while(p_opt[n].opt_name) {
				output(pi_io, "-");
				if(p_opt[n].opt_flags & OF_LONG)
					output(pi_io, "-");
				output(pi_io, p_opt[n].opt_name);
				output(pi_io, "\t\t:");
				if(p_opt[n].opt_help)
					output(pi_io, p_opt[n].opt_help);
				output(pi_io, "\n");

				n++;
			}
		}

		if(p_cmd->cmd_usage) {
			output(pi_io, "\n");
			output(pi_io, "Usage: ");
			output(pi_io, p_cmd->cmd_usage);
			output(pi_io, "\n");
		}
		output(pi_io, "------------------------\n\n");
	}
}

static void help_handler(iCmd *pi_cmd, iCmdHost *pi_cmd_host,
			iIO *pi_io, _cmd_opt_t *p_opt,
			_u32 argc, _cstr_t argv[]) {
	bool bfull = pi_cmd_host->option_check(OPT_FULL, p_opt);
	_cstr_t arg = pi_cmd_host->argument(argc, argv, p_opt, 1);

	output(pi_io, "\n");
	if(arg) {
		// force full help
		bfull = true;
		_cmd_t *p_cmd = pi_cmd_host->get_info(arg);
		if(p_cmd)
			help_command(p_cmd, pi_io, bfull);
		else {
			output(pi_io, "Unknown command '");
			output(pi_io, arg);
			output(pi_io, "\n");
		}
	} else {
		// enum commands
		typedef struct {
			bool bfull;
			iIO *pi_io;
		}_enum_t;

		_enum_t e = {bfull, pi_io};

		pi_cmd_host->enumerate([](_cmd_t *p_cmd, void *udata)->_s32 {
			_enum_t *pe = (_enum_t *)udata;

			help_command(p_cmd, pe->pi_io, pe->bfull);
			return 0;
		}, &e);
	}
}

static _cmd_opt_t _g_help_opt_[] = {
	{OPT_FULL,	0,		0,	"Show full help"},
	{0,		0,		0,	0}
};

static _cmd_t _g_help_cmd_[] = {
	{"help",	_g_help_opt_,	help_handler,
		"Display help information", 	// summary
		0,				// description
		"help [options] [command]"	// usage
	},

	{0,		0,		0,		0}
};

class cCmdHelp: public iCmd {
public:
	BASE(cCmdHelp, "cCmdHelp", RF_ORIGINAL, 1,0,0);

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

static cCmdHelp _g_help_;

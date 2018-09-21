#include <string.h>
#include "iCmd.h"

static void out(iIO *pi_io, _cstr_t str) {
	if(pi_io)
		pi_io->write(str, strlen(str));
}

static void hello_handler(iCmd *pi_cmd, // interface to command opbect
			iCmdHost *pi_cmd_host, // interface to command host
			iIO *pi_io, // interface to I/O object
			_cmd_opt_t *p_opt, // options array
			_u32 argc, // number of arguments
			_str_t argv[] // arguments
			) {
	bool opt_s = pi_cmd_host->option_check("s", p_opt);
	bool opt_1 = pi_cmd_host->option_check("opt-1", p_opt);
	_str_t inp = pi_cmd_host->option_value("inp", p_opt);
	_str_t def = pi_cmd_host->option_value("def", p_opt);
	_u32 arg_idx = 1;
	_str_t arg = pi_cmd_host->argument(argc, argv, p_opt, arg_idx);

	out(pi_io, "Hello !\n");
	if(opt_s)
		out(pi_io, "option 's' present\n");
	if(opt_1)
		out(pi_io, "option 'opt-1' present\n");
	if(inp) {
		out(pi_io, "value of 'inp' option is: ");
		out(pi_io, inp);
		out(pi_io, "\n");
	}
	if(def) {
		out(pi_io, "value of 'def' option is: ");
		out(pi_io, def);
		out(pi_io, "\n");
	}

	if(arg) {
		out(pi_io, "arguments: ");
		while(arg) {
			out(pi_io, arg);
			arg_idx++;
			if((arg = pi_cmd_host->argument(argc, argv, p_opt, arg_idx)))
				out(pi_io, ", ");
		}
		out(pi_io, "\n");
	}
}


static _cmd_opt_t _g_opt[] = {
	{ "s",		0,				0,			"example short option (no value)"},
	{ "opt-1",	OF_LONG,			0,			"example long option (no value)"},
	{ "inp",	OF_LONG|OF_VALUE,		0,			"example input option (--inp=<value>)" },
	{ "def",	OF_LONG|OF_VALUE|OF_PRESENT,	(_str_t)"'Value'",	"example option with default value" },
	{ 0,		0,				0,			0 } // terminate options list
};

static _cmd_t _g_cmd[] = {
	{ "hello",	_g_opt,	hello_handler,	"example command",
		"How to define a new command",
		"hello [option] [arguments]"
	},
	{ 0,	0,	0,	0,	0,	0} // terminate command list
};

class cCmdHello: public iCmd {
public:
	BASE(cCmdHello, "cCmdHello", RF_ORIGINAL, 1,0,0);

	bool object_ctl(_u32 c, void *arg, ...) {
		return true;
	}

	_cmd_t *get_info(void) {
		return _g_cmd;
	}
};

static cCmdHello _g_cmd_hello_;

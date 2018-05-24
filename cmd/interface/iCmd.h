#ifndef __I_CMD_H__
#define __I_CMD_H__

#include "iIO.h"

#define I_CMD		"iCmd"
#define I_CMD_HOST	"iCmdHost"

// option flags
#define OF_VALUE	(1<<0)	// value required
#define OF_LONG		(1<<1)	// flag for long option (lead by --)
#define OF_PRESENT	(1<<2)	// flag for presence by default

typedef struct {
	_cstr_t	opt_name;
	_u32	opt_flags;
	_str_t	opt_value; // default value
	_cstr_t	opt_help;
}_cmd_opt_t;

class iCmd;
class iCmdHost;

typedef void _cmd_handler_t(iCmd *, // interface to command opbect
			iCmdHost *, // interface to command host
			iIO *, // interface to I/O object
			_cmd_opt_t *, // options array
			_u32 argc, // number of arguments
			_str_t argv[] // arguments
			);

typedef struct {
	_cstr_t 	cmd_name;
	_cmd_opt_t	*cmd_options;
	_cmd_handler_t	*cmd_handler;
	_str_t		cmd_usage;
}_cmd_t;

class iCmd: public iBase {
public:
	INTERFACE(iCmd, I_CMD);
	virtual _cmd_t *get_info(void)=0;
};

class iCmdHost: public iBase {
public:
	INTERFACE(iCmdHost, I_CMD_HOST);
	virtual void exec(_str_t, iIO *)=0;
	virtual _cmd_t *get_info(_str_t cmd_name, iCmd **pi_cmd=0)=0;
	// check for option
	virtual bool option_check(_str_t, _cmd_opt_t *)=0;
	// get option value
	virtual _str_t option_value(_str_t, _cmd_opt_t *)=0;
};

#endif


#ifndef __I_ARGS_H__
#define __I_ARGS_H__

#include "iBase.h"

#define I_ARGS	"iArgs"


class iArgs: public iBase {
public:
	INTERFACE(iArgs, I_ARGS);
	virtual void init(_u32 argc, _str_t argv[])=0;
	virtual _cstr_t path(void)=0;
	virtual bool check_option(_char_t opt)=0;
	virtual bool get_option(_char_t opt, _str_t val, _u32 sz)=0;
};

#endif

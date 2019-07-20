#ifndef __I_ARGS_H__
#define __I_ARGS_H__

#include "iBase.h"

#define I_ARGS	"iArgs"


class iArgs: public iBase {
public:
	INTERFACE(iArgs, I_ARGS);
	virtual void init(_u32 argc, _str_t argv[])=0;
	virtual bool init(_cstr_t args)=0;
	virtual _cstr_t path(void)=0;
	virtual _u32 argc(void)=0;
	virtual _str_t *argv(void)=0;
	virtual bool check(_char_t opt)=0;
	virtual bool check(_cstr_t opt)=0;
	virtual _str_t value(_char_t opt)=0;
	virtual _str_t value(_cstr_t opt)=0;
};

#endif

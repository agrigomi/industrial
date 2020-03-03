#ifndef __I_PROCESS_H__
#define __I_PROCESS_H__

#include <signal.h>
#include "iBase.h"

#define I_PROCESS	"iProcess"

#define HPROCESS	void*

class iProcess: public iBase {
public:
	INTERFACE(iProcess, I_PROCESS);

	virtual HPROCESS exec(_cstr_t path, _cstr_t argv[], _cstr_t env[]=NULL)=0;
	virtual bool close(HPROCESS, _s32 signal=SIGINT)=0;
	virtual _s32 read(HPROCESS, _u8 *ptr, _u32 sz)=0;
	virtual _s32 write(HPROCESS, _u8 *ptr, _u32 sz)=0;
	virtual _s32 status(HPROCESS)=0;
	virtual bool wait(HPROCESS)=0;
};

#endif

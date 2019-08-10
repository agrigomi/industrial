#ifndef __I_TASK_MAKER_H__
#define __I_TASK_MAKER_H__

#include "iBase.h"

#define I_TASK_MAKER	"iTaskMaker"

#define TM_SIG_START	1
#define TM_SIG_STOP	2

typedef void*	HTASK;
typedef void *_task_proc_t(_u8 sig, void *);

class iTaskMaker: public iBase {
public:
	INTERFACE(iTaskMaker, I_TASK_MAKER);

	virtual HTASK start(iBase *, void *arg=0)=0;
	virtual HTASK start(_task_proc_t *, void *arg=0)=0;
	virtual HTASK handle(iBase *)=0;
	virtual HTASK handle(_task_proc_t *)=0;
	virtual bool stop(HTASK)=0;
	virtual _err_t join(HTASK)=0;
	virtual _err_t detach(HTASK)=0;
	virtual _err_t set_name(HTASK, _cstr_t name)=0;
	virtual _err_t get_name(HTASK, _str_t name, _u32 len)=0;
};

#endif

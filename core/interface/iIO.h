#ifndef __I_IO_H__
#define __I_IO_H__

#include "iBase.h"

#define I_IO		"iIO"

class iIO: public iBase {
public:
	INTERFACE(iIO, I_IO);
	virtual _u32 read(void *data, _u32 size)=0;
	virtual _u32 write(void *data, _u32 size)=0;
};

#endif


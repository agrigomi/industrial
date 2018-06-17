#ifndef __I_IO_H__
#define __I_IO_H__

#include "iBase.h"

#define I_IO		"iIO"
#define I_STD_IO	"iStdIO"

class iIO: public iBase {
public:
	INTERFACE(iIO, I_IO);
	virtual _u32 read(void *data, _u32 size)=0;
	virtual _u32 write(void *data, _u32 size)=0;
};

class iStdIO: public iIO {
public:
	INTERFACE(iStdIO, I_STD_IO);
	virtual _u32 fwrite(_cstr_t fmt, ...)=0;
	virtual _u32 reads(_str_t str, _u32 size)=0;
};

#endif


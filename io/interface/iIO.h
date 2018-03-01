#ifndef __I_IO_H__
#define __I_IO_H__

#include "iBase.h"

#define I_IO		"iIO"
#define I_FILE_IO	"iFileIO"
#define I_SOCKET_IO	"iSocketIO"

class iIO: public iBase {
public:
	INTERFACE(iIO, I_IO);
	virtual _u32 read(void *data, _u32 size)=0;
	virtual _u32 write(void *data, _u32 size)=0;
};

// mapping protection flags
#define MPF_EXEC	0x4
#define MPF_WRITE	0x2
#define MPF_READ	0x1

// mapping flags
#define MF_SHARED	0x1
#define MF_PRIVATE	0x2
#define MF_FIXED	0x10

class iFileIO: public iIO {
public:
	INTERFACE(iFileIO, I_FILE_IO);
	virtual _ulong seek(_ulong offset)=0
	virtual _ulong size(void)=0;
	virtual void *mmap(_u32 prot=MPF_EXEC|MPF_READ|MPF_WRITE, _u32 flags=MF_SHARED, _ulong offset=0)=0;
	virtual void munmap(void)=0;
};

class iSocketIO: public iIO {
public:
	INTERFACE(iSocketIO, I_SOCKET_IO);
};

#endif


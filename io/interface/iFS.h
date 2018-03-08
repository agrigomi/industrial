#ifndef __I_FS_H__
#define __I_FS_H__

#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include "iIO.h"

#define I_FS	"iFS"

class iFS: public iBase {
public:
	INTERFACE(iFS, I_FS);
	virtual iFileIO *open(_cstr_t path, _u32 flags, _u32 mode=0640)=0;
	virtual void close(iFileIO *)=0;
};

#endif


#ifndef __I_FS_H__
#define __I_FS_H__

#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <dirent.h>
#include "iIO.h"

#define I_FS	"iFS"
#define I_DIR	"iDir"

class iDir: public iBase {
public:
	INTERFACE(iDir, I_DIR);
	virtual bool first(_str_t *fname, _u8 *type)=0;
	virtual bool next(_str_t *fname, _u8 *type)=0;
};

class iFS: public iBase {
public:
	INTERFACE(iFS, I_FS);
	virtual iFileIO *open(_cstr_t path, _u32 flags, _u32 mode=0640)=0;
	virtual void close(iFileIO *)=0;
	virtual iDir *open_dir(_cstr_t path)=0;
	virtual void close_dir(iDir *)=0;
	virtual bool access(_cstr_t path, _u32 mode)=0;
	virtual bool remove(_cstr_t path)=0;
};

#endif


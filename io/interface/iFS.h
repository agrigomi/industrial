#ifndef __I_FS_H__
#define __I_FS_H__

#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <dirent.h>
#include <time.h>
#include "iIO.h"

#define I_FILE_IO	"iFileIO"
#define I_FS		"iFS"
#define I_DIR		"iDir"

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
	virtual _ulong seek(_ulong offset)=0;
	virtual _ulong size(void)=0;
	virtual void *map(_u32 prot=MPF_EXEC|MPF_READ|MPF_WRITE, _u32 flags=MF_SHARED)=0;
	virtual void unmap(void)=0;
	virtual void sync(void)=0;
	virtual mode_t mode(void)=0;
	virtual bool truncate(_ulong len)=0;
	virtual time_t access_time(void)=0; // last access timestamp
	virtual time_t modify_time(void)=0; // ladst modification timestamp
	virtual uid_t user_id(void)=0;
	virtual gid_t group_id(void)=0;
};

class iDir: public iBase {
public:
	INTERFACE(iDir, I_DIR);
	virtual bool first(_str_t *fname, _u8 *type)=0;
	virtual bool next(_str_t *fname, _u8 *type)=0;
};

class iFS: public iBase {
public:
	INTERFACE(iFS, I_FS);
	virtual iFileIO *create(_cstr_t path, _u32 flags=O_RDWR|O_TRUNC|O_CREAT, _u32 mode=S_IRWXU)=0;
	virtual iFileIO *open(_cstr_t path, _u32 flags=O_RDWR)=0;
	virtual void close(iFileIO *)=0;
	virtual iDir *open_dir(_cstr_t path)=0;
	virtual bool mk_dir(_cstr_t path, _u32 mode=S_IRWXU)=0;
	virtual void close_dir(iDir *)=0;
	virtual bool access(_cstr_t path, _u32 mode=F_OK)=0;
	virtual mode_t mode(_cstr_t path)=0;
	virtual bool remove(_cstr_t path)=0;
	virtual _ulong size(_cstr_t path)=0;
	virtual time_t access_time(_cstr_t path)=0;
	virtual time_t modify_time(_cstr_t path)=0;
	virtual uid_t user_id(_cstr_t path)=0;
	virtual gid_t group_id(_cstr_t path)=0;
	virtual bool copy(_cstr_t src, _cstr_t dst)=0;
};

#endif


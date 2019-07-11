#ifndef __I_FS_H__
#define __I_FS_H__

#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/mman.h>
#include <dirent.h>
#include <time.h>
#include "iIO.h"
#include "iMemory.h"

#define I_FILE_IO	"iFileIO"
#define I_FS		"iFS"
#define I_DIR		"iDir"
#define I_FILE_CACHE	"iFileCache"

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

#define HFCACHE	void*

class iFileCache: public iBase {
public:
	INTERFACE(iFileCache, I_FILE_CACHE);
	// Initialize cache with path to cache folder
	virtual bool init(_cstr_t path, _cstr_t key=NULL, iHeap *pi_heap=NULL)=0;
	// open file in cache
	virtual HFCACHE open(_cstr_t path)=0;
	// return pointer to file content
	virtual void *ptr(HFCACHE hfc, _ulong *size)=0;
	// close file in cache
	virtual void close(HFCACHE hfc)=0;
	// return last modify time
	virtual time_t mtime(HFCACHE hfc)=0;
	// remove cache
	virtual void remove(HFCACHE hfc)=0;
};

class iFS: public iBase {
public:
	INTERFACE(iFS, I_FS);
	virtual iFileIO *create(_cstr_t path, _u32 flags=O_RDWR|O_TRUNC|O_CREAT, _u32 mode=S_IRWXU)=0;
	virtual iFileIO *open(_cstr_t path, _u32 flags=O_RDWR)=0;
	virtual void close(iFileIO *)=0;
	virtual iDir *open_dir(_cstr_t path)=0;
	virtual bool mk_dir(_cstr_t path, _u32 mode=S_IRWXU)=0;
	virtual bool rm_dir(_cstr_t path)=0;
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


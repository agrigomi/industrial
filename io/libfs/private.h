#ifndef __FS_PRIVATE_H__
#define __FS_PRIVATE_H__

#include "iFS.h"

#define FILE_IO_CLASS_NAME	"cFileIO"
#define DIR_CLASS_NAME		"cDir"


class cDir: public iDir {
private:
	DIR *mp_dir;
	off_t m_off_first;
public:

	BASE(cDir, DIR_CLASS_NAME, RF_CLONE, 1,0,0);
	bool object_ctl(_u32 cmd, void *arg, ...);
	bool first(_str_t *fname, _u8 *type);
	bool next(_str_t *fname, _u8 *type);
	void _init(DIR *p_dir);
	void _close(void);
};

class cFileIO: public iFileIO {
private:
	void *m_map_addr;
	_ulong m_map_len;
public:
	_s32	m_fd;

	BASE(cFileIO, FILE_IO_CLASS_NAME, RF_CLONE, 1,0,0);
	bool object_ctl(_u32 cmd, void *arg, ...);
	_u32 read(void *data, _u32 size);
	_u32 write(const void *data, _u32 size);
	_ulong seek(_ulong offset);
	_ulong size(void);
	void *map(_u32 prot=MPF_EXEC|MPF_READ|MPF_WRITE, _u32 flags=MF_SHARED);
	void unmap(void);
	void sync(void);
	mode_t mode(void);
	bool truncate(_ulong len);
	void _close(void);
	time_t access_time(void); // last access timestamp
	time_t modify_time(void); // ladst modification timestamp
	uid_t user_id(void);
	gid_t group_id(void);
};

#endif

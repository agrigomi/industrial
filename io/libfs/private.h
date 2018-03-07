#ifndef __FS_PRIVATE_H__
#define __FS_PRIVATE_H__

#include "iFS.h"

#define FILE_IO_CLASS_NAME	"cFileIO"

class cFileIO: public iFileIO {
private:
	void *m_map_addr;
	_ulong m_map_len;
public:
	_s32	m_fd;

	BASE(cFileIO, FILE_IO_CLASS_NAME, RF_CLONE, 1,0,0);
	bool object_ctl(_u32 cmd, void *arg, ...);
	_ulong seek(_ulong offset);
	_ulong size(void);
	void *map(_u32 prot=MPF_EXEC|MPF_READ|MPF_WRITE, _u32 flags=MF_SHARED);
	void unmap(void);
	void sync(void);
	bool truncate(_ulong len);
	void _close(void);
};

#endif

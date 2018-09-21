#include <sys/mman.h>
#include "private.h"

bool cFileIO::object_ctl(_u32 cmd, void *arg, ...) {
	bool r = false;

	switch(cmd) {
		case OCTL_INIT:
			m_fd = 0;
			m_map_addr = 0;
			m_map_len = 0;
			r = true;
			break;
		case OCTL_UNINIT:
			_close();
			r = true;
			break;
	}

	return r;
}

_u32 cFileIO::read(void *data, _u32 size) {
	_u32 r = 0;

	if(m_fd > 0)
		r = ::read(m_fd, data, size);

	return r;
}

_u32 cFileIO::write(const void *data, _u32 size) {
	_u32 r = 0;

	if(m_fd > 0)
		r = ::write(m_fd, data, size);

	return r;
}

_ulong cFileIO::seek(_ulong offset) {
	_ulong r = -1;
	if(m_fd > 0)
		r = lseek(m_fd, offset, SEEK_SET);
	return r;
}

_ulong cFileIO::size(void) {
	_ulong r = 0;

	if(m_fd > 0) {
		// get current position
		_ulong _r = lseek(m_fd, 0, SEEK_CUR);
		// get file len
		r = lseek(m_fd, 0, SEEK_END);
		// restore opsition
		lseek(m_fd, _r, SEEK_SET);
	}

	return r;
}

void cFileIO::_close(void) {
	if(m_fd > 0) {
		unmap();
		if(::close(m_fd) != -1)
			m_fd = 0;
	}
}

void *cFileIO::map(_u32 prot, _u32 flags) {
	void *r = 0;

	if(m_fd > 0) {
		if((m_map_len = size()))
			r = m_map_addr = mmap(0, m_map_len, prot, flags, m_fd, 0);
	}

	return r;
}

void cFileIO::unmap(void) {
	if(m_fd > 0 && m_map_addr && m_map_len) {
		if(munmap(m_map_addr, m_map_len) != -1) {
			m_map_len = 0;
			m_map_addr = 0;
		}
	}
}

void cFileIO::sync(void) {
	if(m_fd > 0)
		fsync(m_fd);
}

bool cFileIO::truncate(_ulong len) {
	bool r = false;

	if(m_fd > 0) {
		if(ftruncate(m_fd, len) != -1)
			r = true;
	}

	return r;
}

static cFileIO _g_file_io_;

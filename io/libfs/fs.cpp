#include "iFS.h"
#include "iRepository.h"
#include "private.h"
#include "startup.h"

IMPLEMENT_BASE_ARRAY("fs", 3);

class cFS: public iFS {
public:
	BASE(cFS, "cFS", RF_ORIGINAL, 1,0,0);

	bool object_ctl(_u32 cmd, void *arg, ...) {
		bool r = false;

		switch(cmd) {
			case OCTL_INIT:
				r = true;
				break;
			case OCTL_UNINIT:
				r = true;
				break;
		}

		return r;
	}

	iFileIO *open(_cstr_t path, _u32 flags, _u32 mode) {
		iFileIO *r = 0;

		_s32 fd = ::open(path, flags, mode);
		if(fd > 0) {
			cFileIO *_r = (cFileIO *)_gpi_repo_->object_by_cname(FILE_IO_CLASS_NAME, RF_CLONE);
			if(_r) {
				_r->m_fd = fd;
				r = _r;
			} else
				::close(fd);
		}

		return r;
	}

	void close(iFileIO *pi) {
		cFileIO *cfio = dynamic_cast<cFileIO*>(pi);
		if(cfio) {
			cfio->_close();
			_gpi_repo_->object_release(pi);
		}
	}

	iDir *open_dir(_cstr_t path) {
		iDir *r = 0;
		DIR *p_dir = opendir(path);

		if(p_dir) {
			cDir *pc_dir = (cDir *)_gpi_repo_->object_by_cname(DIR_CLASS_NAME, RF_CLONE);
			if(pc_dir) {
				pc_dir->_init(p_dir);
				r = pc_dir;
			} else
				closedir(p_dir);
		}

		return r;
	}

	void close_dir(iDir *pi) {
		cDir *pc_dir = dynamic_cast<cDir*>(pi);
		if(pc_dir) {
			pc_dir->_close();
			_gpi_repo_->object_release(pi);
		}
	}

	bool access(_cstr_t path, _u32 mode) {
		bool r = false;

		if(access(path, mode) == 0)
			r = true;

		return r;
	}

	bool remove(_cstr_t path) {
		bool r = false;

		if(unlink(path) == 0)
			r = true;

		return r;
	}
};

static cFS _g_fs_;


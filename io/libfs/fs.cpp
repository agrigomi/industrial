#include "iFS.h"
#include "iRepository.h"
#include "private.h"
#include "startup.h"

IMPLEMENT_BASE_ARRAY("fs", 10);

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

	iFileIO *create(_cstr_t path, _u32 flags=O_RDWR|O_TRUNC|O_CREAT, _u32 mode=S_IRWXU) {
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

	iFileIO *open(_cstr_t path, _u32 flags=O_RDWR) {
		iFileIO *r = 0;

		_s32 fd = ::open(path, flags);
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

	bool mk_dir(_cstr_t path, _u32 mode=S_IRWXU) {
		return (mkdir(path, mode) == 0) ? true : false;
	}

	bool rm_dir(_cstr_t path) {
		return (rmdir(path) == 0) ? true : false;
	}

	void close_dir(iDir *pi) {
		cDir *pc_dir = dynamic_cast<cDir*>(pi);
		if(pc_dir) {
			pc_dir->_close();
			_gpi_repo_->object_release(pi);
		}
	}

	bool access(_cstr_t path, _u32 mode=F_OK) {
		bool r = false;

		if(::access(path, mode) == 0)
			r = true;

		return r;
	}

	mode_t mode(_cstr_t path) {
		mode_t r = 0;
		struct stat st;

		if(stat(path, &st) == 0)
			r = st.st_mode;

		return r;
	}

	bool remove(_cstr_t path) {
		bool r = false;

		if(unlink(path) == 0)
			r = true;

		return r;
	}

	_ulong size(_cstr_t path) {
		_ulong r = 0;
		struct stat st;

		if(stat(path, &st) == 0)
			r = st.st_size;

		return r;
	}

	time_t access_time(_cstr_t path) {
		time_t r = 0;
		struct stat st;

		if(stat(path, &st) == 0)
			r = st.st_atime;

		return r;
	}

	time_t modify_time(_cstr_t path) {
		time_t r = 0;
		struct stat st;

		if(stat(path, &st) == 0)
			r = st.st_mtime;

		return r;
	}

	uid_t user_id(_cstr_t path) {
		uid_t r = 0;
		struct stat st;

		if(stat(path, &st) == 0)
			r = st.st_uid;

		return r;
	}

	gid_t group_id(_cstr_t path) {
		gid_t r = 0;
		struct stat st;

		if(stat(path, &st) == 0)
			r = st.st_gid;

		return r;
	}

	bool copy(_cstr_t src, _cstr_t dst) {
		bool r = false;
		iFileIO *pi_src = open(src, O_RDONLY);

		if(pi_src) {
			iFileIO *pi_dst = create(dst, O_CREAT|O_RDWR|O_TRUNC, pi_src->mode());
			if(pi_dst) {
				void *src_ptr = pi_src->map(MPF_READ);

				if(src_ptr) {
					pi_dst->write(src_ptr, pi_src->size());
					r = true;
				}

				close(pi_dst);
			}

			close(pi_src);
		}

		return r;
	}
};

static cFS _g_fs_;


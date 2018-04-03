#include "private.h"

bool cDir::object_ctl(_u32 cmd, void *arg, ...) {
	bool r = false;

	switch(cmd) {
		case OCTL_INIT:
			mp_dir = 0;
			m_off_first = 0;
			r = true;
			break;
		case OCTL_UNINIT:
			_close();
			r = true;
			break;
	}

	return r;
}

void cDir::_init(DIR *p_dir) {
	mp_dir = p_dir;

	m_off_first = telldir(p_dir);
}

void cDir::_close(void) {
	if(mp_dir) {
		closedir(mp_dir);
		mp_dir = 0;
	}
}

bool cDir::first(_str_t *fname, _u8 *type) {
	bool r = false;

	if(mp_dir) {
		dirent *p_dentry = 0;

		seekdir(mp_dir, m_off_first);
		if((p_dentry = readdir(mp_dir))) {
			*fname = p_dentry->d_name;
			*type = p_dentry->d_type;
			r = true;
		}
	}

	return r;
}

bool cDir::next(_str_t *fname, _u8 *type) {
	bool r = false;

	if(mp_dir) {
		dirent *p_dentry = 0;

		if((p_dentry = readdir(mp_dir))) {
			*fname = p_dentry->d_name;
			*type = p_dentry->d_type;
			r = true;
		}
	}

	return r;
}

static cDir _g_dir_;

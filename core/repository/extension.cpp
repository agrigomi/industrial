#include <string.h>
#include <link.h>
#include <dlfcn.h>
#include <stdlib.h>
#include "private.h"

#define MAX_ALIAS_LEN	32

typedef void*	_dl_handle_t;
typedef _base_vector_t *_get_base_vector_t(void);
typedef void _init_t(iRepository *);

class cRepoExtension: public iRepoExtension {
private:
	_s8 m_alias[MAX_ALIAS_LEN];
	_dl_handle_t m_handle;
	_get_base_vector_t *m_get_base_vector;
	_init_t *m_init;
public:
	BASE(cRepoExtension, "cRepoExtension", RF_CLONE, 1, 0,0);

	bool object_ctl(_u32 cmd, void *arg) {
		bool r = false;
		switch(cmd) {
			case OCTL_INIT: {
				memset(m_alias, 0, sizeof(m_alias));
				m_handle = 0;
				m_get_base_vector = 0;
				m_init = 0;
				r = true;
				break;
			}
			case OCTL_UNINIT: {
				unload();
				r = true;
				break;
			}
		}
		return r;
	}

	_str_t alias(void) {
		return m_alias;
	}

	_err_t load(_str_t file, _str_t alias=0) {
		_err_t r = ERR_UNKNOWN;
		if((m_handle = dlopen(file, RTLD_LAZY))) {
			m_init = (_init_t *)dlsym(m_handle, "init");
			m_get_base_vector = (_get_base_vector_t *)dlsym(m_handle, "get_base_vector");
			if(m_get_base_vector && m_init ) {
				strncpy(m_alias, (alias)?alias:basename(file), sizeof(m_alias));
				r = ERR_NONE;
			} else
				dlclose(m_handle);
		} else
			r = ERR_LOADEXT;
		return r;
	}

	void init(iRepository *pi_repo) {
		if(m_handle && m_init)
			m_init(pi_repo);
	}

	_err_t unload(void) {
		_err_t r = ERR_UNKNOWN;
		if(m_handle) {
			if(dlclose(m_handle) == 0)
				r = ERR_NONE;
		}
		return r;
	}

	_str_t file(void) {
		_str_t r = 0;
		if(m_handle) {
			link_map info;
			if(dlinfo(m_handle, RTLD_DI_LINKMAP, &info) == 0)
				r = info.l_name;
		}
		return r;
	}

	void *address(void) {
		void *r = 0;
		//...
		return r;
	}

	_base_vector_t *vector(void) {
		_base_vector_t *r = 0;
		if(m_handle) {
			if(m_get_base_vector)
				r = m_get_base_vector();
		}
		return r;
	}
};

static cRepoExtension _g_object_;

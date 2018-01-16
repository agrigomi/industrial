#include <string.h>
#include <link.h>
#include <dlfcn.h>
#include "private.h"

#define MAX_ALIAS_LEN	32

typedef void*	_dl_handle_t;

class cRepoExtension: public iRepoExtension {
private:
	_s8 m_alias[MAX_ALIAS_LEN];
	_dl_handle_t m_handle;

public:
	BASE(cRepoExtension, "cRepoExtension", RF_CLONE, 1, 0,0);

	bool object_ctl(_u32 cmd, void *arg) {
		bool r = false;
		switch(cmd) {
			case OCTL_INIT: {
				memset(m_alias, 0, sizeof(m_alias));
				m_handle = 0;
				r = true;
				break;
			}
			case OCTL_UNINIT: {
				unload();
				//...
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
			//...
		} else
			r = ERR_LOADEXT;
		return r;
	}

	_err_t unload(void) {
		_err_t r = ERR_UNKNOWN;
		//...
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

	void *vector(void) {
		void *r = 0;
		//...
		return r;
	}
};

static cRepoExtension _g_object_;

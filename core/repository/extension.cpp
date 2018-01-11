#include <string.h>
#include "private.h"

#define MAX_ALIAS_LEN	32

class cRepoExtension: public iRepoExtension {
private:
	_str_t m_alias[MAX_ALIAS_LEN];
public:
	BASE(cRepoExtension, "cRepoExtension", RF_CLONE, 1, 0,0);

	bool object_ctl(_u32 cmd, void *arg) {
		switch(cmd) {
			case OCTL_INIT: {
				memset(m_alias, 0, sizeof(m_alias));
				//...
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
	}

	_str_t alias(void) {
		return m_alias;
	}

	_err_t load(_str_t file, _str_t alias=0) {
		_err_t r = ERR_UNKNOWN;
		//...
		return r;
	}

	_err_t unload(void) {
		_err_t r = ERR_UNKNOWN;
		//...
		return r;
	}
};

static cRepoExtension _g_object_;

#include <string.h>
#include "iArgs.h"
#include "iSync.h"
#include "iRepository.h"

#define OPT_VALUE	(1<<0)
#define OPT_LONG	(1<<1)


class cArgs: public iArgs {
private:
	_u32 m_argc;
	_str_t *m_argv;

	bool check(_cstr_t opt, _str_t arg, _u32 flags, _u32 *idx) {
		bool r = false;

		if(arg[0] == '-') {
			_u32 len = strlen(arg);
			_u32 i = 1;
			if(!(flags & OPT_LONG)) {
				for(; i < len; i++) {
					_char_t special[]="-=:";
					_u32 l = strlen(special);
					_u32 j = 0;
					for(; j < l; j++) {
						if(arg[i] == special[j])
							break;
					}
					if(j < l)
						break;
					if(opt[0] == arg[i]) {
						r = true;
						*idx = i;
						break;
					}
				}
			} else { // parse long option
				if(arg[i] == '-')
					i++;

				_u32 j = 0;
				for(; i < len; i++, j++) {
					if(opt[j] != arg[i])
						break;
				}
				if(j == strlen(opt) && (arg[i] == 0 || arg[i] == '=')) {
					*idx = i;
					r = true;
				}
			}
		}

		return r;
	}

public:
	BASE(cArgs, "cArgs", RF_ORIGINAL | RF_CLONE, 1,0,0);

	bool object_ctl(_u32 cmd, void *arg, ...) {
		bool r = false;
		switch(cmd) {
			case OCTL_INIT:
				m_argc = 0;
				m_argv = 0;
				r = true;
				break;
			case OCTL_UNINIT:
				r = true;
				break;
		}
		return r;
	}

	void init(_u32 argc, _str_t argv[]) {
		m_argc = argc;
		m_argv = argv;
	}

	bool init(_cstr_t args) {
		bool r = false;

		//...

		return r;
	}

	_cstr_t path(void) {
		_cstr_t r = 0;
		if(m_argc && m_argv)
			r = m_argv[0];
		return r;
	}

	_u32 argc(void) {
		return m_argc;
	}

	_str_t *argv(void) {
		return m_argv;
	}

	bool check(_char_t opt) {
		bool r = false;
		_char_t sopt[2]="";
		_u32 idx=0;

		sopt[0] = opt;
		if(m_argc > 1 && m_argv) {
			for(_u32 i = 1; i < m_argc; i++) {
				if((r = check(sopt, m_argv[i], 0, &idx)))
					break;
			}
		}
		return r;
	}

	bool check(_cstr_t opt) {
		bool r = false;
		_u32 idx = 0;

		if(m_argc > 1 && m_argv) {
			for(_u32 i = 1; i < m_argc; i++) {
				if((r = check(opt, m_argv[i], OPT_LONG, &idx)))
					break;
			}
		}
		return r;
	}

	_str_t value(_char_t opt) {
		_str_t r = 0;
		_char_t sopt[2]="";

		sopt[0] = opt;
		if(m_argc > 1 && m_argv) {
			_u32 idx = 0;
			for(_u32 i = 1; i < m_argc; i++) {
				if(check(sopt, m_argv[i], OPT_VALUE, &idx)) {
					if(idx+1 < (_u32)strlen(m_argv[i])) {
						r = m_argv[i] + idx+1;
						break;
					} else if(i+1 < m_argc && m_argv[i+1][0] != '-') {
						r = m_argv[i+1];
						break;
					}
				}
			}
		}
		return r;
	}

	_str_t value(_cstr_t opt) {
		_str_t r = 0;

		if(m_argc > 1 && m_argv) {
			_u32 idx = 0;
			for(_u32 i = 1; i < m_argc; i++) {
				if(check(opt, m_argv[i], OPT_VALUE|OPT_LONG, &idx)) {
					if(m_argv[i][idx] == '=') {
						r = m_argv[i] + idx+1;
						break;
					} else if(i+1 < m_argc && m_argv[i+1][0] != '-') {
						r = m_argv[i+1];
						break;
					}
				}
			}
		}

		return r;
	}
};

static cArgs _g_object_;

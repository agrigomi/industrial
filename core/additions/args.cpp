#include <string.h>
#include "iArgs.h"
#include "iSync.h"
#include "iRepository.h"
#include "iMemory.h"
#include "iStr.h"

#define OPT_VALUE	(1<<0)
#define OPT_LONG	(1<<1)

#define MAX_ARGV 128

class cArgs: public iArgs {
private:
	_u32 m_argc;
	_str_t *m_argv;
	iStr	*mpi_str;
	iHeap	*mpi_heap;
	_str_t	m_arg_line;
	_u32 	m_sz_arg_line;
	_str_t	m_arg_array[MAX_ARGV];

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

#define INV_POS  0xffffffff

	// converts a command line string to list of arguments
	_u32 parse_argv(_str_t cmd_line, _u32 cmd_len) {
		_u32 r = 0;
		bool strophe = false;
		bool quotes = false;
		_u32 arg_pos = INV_POS;
		_u32 i = 0;

		while(i < cmd_len + 1 && r < MAX_ARGV) {
			switch(cmd_line[i]) {
				case '\'':
					mpi_str->mem_cpy(cmd_line+i, cmd_line+i+1, cmd_len - i);
					strophe = !strophe;
					continue;
				case '"':
					mpi_str->mem_cpy(cmd_line+i, cmd_line+i+1, cmd_len - i);
					quotes = !quotes;
					continue;
				case '\\':
					mpi_str->mem_cpy(cmd_line+i, cmd_line+i+1, cmd_len - i);
					break;
				case 0:
				case ' ':
				case '\n':
				case '\r':
				case '\t':
					if(arg_pos != INV_POS) {
						if(!quotes && !strophe) {
							m_arg_array[r] = cmd_line + arg_pos;
							arg_pos = INV_POS;
							cmd_line[i] = 0;
							r++;
						}
					}
					break;
				default:
					if(arg_pos == INV_POS)
						arg_pos = i;
					break;
			}

			i++;
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
				m_arg_line = NULL;
				m_sz_arg_line = 0;
				mpi_str = dynamic_cast<iStr *>(_gpi_repo_->object_by_iname(I_STR, RF_ORIGINAL));
				mpi_heap = dynamic_cast<iHeap *>(_gpi_repo_->object_by_iname(I_HEAP, RF_ORIGINAL));

				if(mpi_str && mpi_heap)
					r = true;
				break;
			case OCTL_UNINIT:
				if(m_arg_line && m_sz_arg_line)
					mpi_heap->free(m_arg_line, m_sz_arg_line);
				_gpi_repo_->object_release(mpi_str);
				_gpi_repo_->object_release(mpi_heap);
				r = true;
				break;
		}
		return r;
	}

	void init(_u32 argc, _str_t argv[]) {
		m_argc = argc;
		m_argv = argv;
	}

	bool init(_cstr_t args, _u32 sz_args=0) {
		bool r = false;
		_u32 sz = (sz_args) ? sz_args : strlen(args);

		if(sz) {
			if(m_arg_line && m_sz_arg_line)
				mpi_heap->free(m_arg_line, m_sz_arg_line);

			if((m_arg_line = (_str_t)mpi_heap->alloc(sz+1))) {
				mpi_str->mem_set(m_arg_line, 0, sz+1);
				mpi_str->mem_cpy(m_arg_line, (void *)args, sz);
				m_sz_arg_line = sz;
				m_argc = parse_argv(m_arg_line, sz);
				m_argv = m_arg_array;
				r = true;
			}
		}

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
			for(_u32 i = 0; i < m_argc; i++) {
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
			for(_u32 i = 0; i < m_argc; i++) {
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

			for(_u32 i = 0; i < m_argc; i++) {
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

			for(_u32 i = 0; i < m_argc; i++) {
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

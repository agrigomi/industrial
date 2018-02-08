#include <getopt.h>
#include <string.h>
#include "iArgs.h"
#include "iSync.h"
#include "iRepository.h"

struct args_context {
	_u32 optind;
	_s32 opterr;
	_u32 optopt;

	args_context() {
		optind = 0;
		opterr = 0;
		optopt = 0;
	}
};

typedef args_context _args_context_t;

class cArgs: public iArgs {
private:
	_u32 m_argc;
	_str_t *m_argv;
	iMutex *mpi_mutex;

	HMUTEX lock(void) {
		HMUTEX r = 0;
		if(mpi_mutex)
			r = mpi_mutex->lock();
		return r;
	}

	void unlock(HMUTEX hm) {
		if(mpi_mutex)
			mpi_mutex->unlock(hm);
	}

	_char_t parse(_cstr_t opt, _str_t arg=0, _u32 sz_arg=0, _args_context_t *cxt=0) {
		_char_t r = 0;
		HMUTEX hm = lock();

		if(cxt) { // restore context
			optind = cxt->optind;
			opterr = cxt->opterr;
			optopt = cxt->optopt;
		}

		if((r = (_char_t)getopt(m_argc, m_argv, opt)) != -1) {
			if(arg && optarg)
				strncpy(arg, optarg, sz_arg);
		} else
			r = 0;

		if(cxt) { // save context
			cxt->optind = optind;
			cxt->opterr = opterr;
			cxt->optopt = optopt;
		}

		unlock(hm);
		return r;
	}

public:
	BASE(cArgs, "cArgs", RF_ORIGINAL, 1,0,0);

	bool object_ctl(_u32 cmd, void *arg) {
		bool r = false;
		switch(cmd) {
			case OCTL_INIT: {
				m_argc = 0;
				m_argv = 0;
				iRepository *pi_repo = (iRepository*)arg;
				if((mpi_mutex = (iMutex*)pi_repo->object_by_iname(I_MUTEX, RF_CLONE)))
					r = true;
				break;
			}
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

	_cstr_t path(void) {
		_cstr_t r = 0;
		if(m_argc)
			r = m_argv[0];
		return r;
	}

	bool check_option(_char_t opt) {
		bool r = false;
		_char_t sopt[2]="";
		_args_context_t cxt;
		sopt[0] = opt;
		_char_t x;

		while((x = parse(sopt, 0, 0, &cxt))) {
			if(x == opt) {
				r = true;
				break;
			}
		}

		return r;
	}

	bool get_option(_char_t opt, _str_t val, _u32 sz) {
		bool r = false;
		_char_t sopt[3]="";
		_args_context_t cxt;
		sopt[0] = opt;
		sopt[1] = ':';
		_char_t x;

		while((x = parse(sopt, val, sz, &cxt))) {
			if(x == opt) {
				r = true;
				break;
			}
		}

		return r;
	}
};

static cArgs _g_object_;

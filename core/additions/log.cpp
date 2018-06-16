#include <stdio.h>
#include <stdlib.h>
#include "iRepository.h"
#include "iMemory.h"
#include "iLog.h"
#include "iSync.h"
#include "iArgs.h"

#define MAX_MSG_BUFFER		1024
#define DEFAULT_RB_CAPACITY 	16384

class cLog: public iLog {
private:
	iRingBuffer *mpi_rb;
	iLlist *mpi_lstr;

	void sync(void) {
		_str_t msg = 0;

		if(mpi_rb && mpi_lstr && mpi_lstr->cnt()) {
			_u16 sz = 0;

			while((msg = (_str_t)mpi_rb->pull(&sz))) {
				HMUTEX hm = mpi_lstr->lock();
				_u32 sz = 0;
				_log_listener_t **plstr = (_log_listener_t**)mpi_lstr->first(&sz, hm);
				if(plstr) {
					do {
						_log_listener_t *f = *plstr;
						f(msg[0], msg+1);
					}while((plstr = (_log_listener_t**)mpi_lstr->next(&sz, hm)));
				}
				mpi_lstr->unlock(hm);
			}
		}
	}

	void sync(_log_listener_t *lstr) {
		if(mpi_rb) {
			_str_t msg = 0;
			_u16 sz = 0;

			mpi_rb->reset_pull();
			while((msg = (_str_t)mpi_rb->pull(&sz))) {
				if(sz)
					lstr(msg[0], msg+1);
			}
		}
	}

	void init_rb(iRepository *pi_repo) {
		iArgs *pi_args = (iArgs*)pi_repo->object_by_iname(I_ARGS, RF_ORIGINAL);
		_u32 lbc = DEFAULT_RB_CAPACITY;

		if(pi_args) {
			_str_t log_buffer = pi_args->value("log-buffer");
			if(log_buffer)
				lbc = atoi(log_buffer);
			pi_repo->object_release(pi_args);
		}
		mpi_rb->init(lbc);
	}

public:
	BASE(cLog, "cLog", RF_CLONE|RF_ORIGINAL, 1,0,0);

	bool object_ctl(_u32 cmd, void *arg, ...) {
		bool r = false;

		switch(cmd) {
			case OCTL_INIT: {
				mpi_rb = 0;
				mpi_lstr = 0;
				iRepository *pi_repo = (iRepository*)arg;
				mpi_rb = (iRingBuffer*)pi_repo->object_by_iname(I_RING_BUFFER, RF_CLONE);
				mpi_lstr = (iLlist*)pi_repo->object_by_iname(I_LLIST, RF_CLONE);
				if(mpi_rb && mpi_lstr) {
					mpi_lstr->init(LL_VECTOR, 1);
					init_rb(pi_repo);
					r = true;
				}
				break;
			}
			case OCTL_UNINIT: {
				iRepository *pi_repo = (iRepository*)arg;
				pi_repo->object_release(mpi_rb);
				pi_repo->object_release(mpi_lstr);
				r = true;
				break;
			}
		}

		return r;
	}

	HMUTEX lock(HMUTEX hm=0) {
		HMUTEX r = 0;
		if(mpi_lstr)
			r = mpi_lstr->lock(hm);
		return r;
	}

	void unlock(HMUTEX hm) {
		if(mpi_lstr)
			mpi_lstr->unlock(hm);
	}

	void add_listener(_log_listener_t *lstr) {
		HMUTEX hm = lock();
		bool add = true;
		_u32 sz;

		_log_listener_t **plstr = (_log_listener_t **)mpi_lstr->first(&sz, hm);
		if(plstr) {
			do {
				if(*plstr == lstr) {
					add = false;
					break;
				}
			}while((plstr = (_log_listener_t **)mpi_lstr->next(&sz, hm)));
		}

		if(add) {
			mpi_lstr->add(&lstr, sizeof(lstr), hm);
			sync(lstr);
		}

		unlock(hm);
	}

	void remove_listener(_log_listener_t *lstr) {
		_u32 sz;
		HMUTEX hm = lock();
		_log_listener_t **plstr = (_log_listener_t **)mpi_lstr->first(&sz, hm);

		if(plstr) {
			do {
				if(lstr == *plstr) {
					mpi_lstr->del(hm);
					break;
				}
			} while((plstr = (_log_listener_t **)mpi_lstr->next(&sz, hm)));
		}
		unlock(hm);
	}

	void write(_u8 lmt, _cstr_t msg) {
		if(mpi_rb) {
			_s8 _msg[MAX_MSG_BUFFER];

			_u32 sz = snprintf(_msg+1, sizeof(_msg)-1, "%s", msg);
			_msg[0] = lmt;
			mpi_rb->push(_msg, sz+2);
			sync();
		}
	}

	void fwrite(_u8 lmt, _cstr_t fmt, ...) {
		if(mpi_rb) {
			_s8 _msg[MAX_MSG_BUFFER];
			va_list args;

			va_start(args, fmt);
			_u32 sz = vsnprintf(_msg+1, sizeof(_msg)-1, fmt, args);
			_msg[0] = lmt;
			mpi_rb->push(_msg, sz+2);
			va_end(args);
			sync();
		}
	}

	_str_t first(HMUTEX hm) {
		_str_t r = 0;
		_u16 sz = 0;

		if(mpi_rb) {
			HMUTEX h = lock(hm);

			mpi_rb->reset_pull();
			if((r = (_str_t)mpi_rb->pull(&sz)))
				r++;
			unlock(h);
		}
		return r;
	}

	_str_t next(HMUTEX hm) {
		_str_t r = 0;
		_u16 sz = 0;

		if(mpi_rb) {
			HMUTEX h = lock(hm);

			if((r = (_str_t)mpi_rb->pull(&sz)))
				r++;

			unlock(h);
		}
		return r;
	}
};

static cLog _g_object_;

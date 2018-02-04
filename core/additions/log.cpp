#include <stdio.h>
#include "iRepository.h"
#include "iMemory.h"
#include "iLog.h"
#include "iSync.h"

#define MAX_MSG_BUFFER	1024

class cLog: public iLog {
private:
	iRingBuffer *mpi_rb;
	iMutex *mpi_mutex;
	iLlist *mpi_lstr;

	void sync(void) {
	}

public:
	BASE(cLog, "cLog", RF_CLONE|RF_ORIGINAL, 1,0,0);

	bool object_ctl(_u32 cmd, void *arg) {
		bool r = false;

		switch(cmd) {
			case OCTL_INIT: {
				mpi_rb = 0;
				mpi_mutex = 0;
				mpi_lstr = 0;
				iRepository *pi_repo = (iRepository*)arg;
				mpi_rb = (iRingBuffer*)pi_repo->object_by_iname(I_RING_BUFFER, RF_CLONE);
				mpi_mutex = (iMutex*)pi_repo->object_by_iname(I_MUTEX, RF_CLONE);
				mpi_lstr = (iLlist*)pi_repo->object_by_iname(I_LLIST, RF_CLONE);
				if(mpi_rb && mpi_mutex && mpi_lstr)
					r = true;
				break;
			}
			case OCTL_UNINIT: {
				iRepository *pi_repo = (iRepository*)arg;
				pi_repo->object_release(mpi_rb);
				pi_repo->object_release(mpi_lstr);
				pi_repo->object_release(mpi_mutex);
				r = true;
				break;
			}
		}

		return r;
	}

	void init(_u32 capacity) {
		if(mpi_rb)
			mpi_rb->init(capacity);
	}

	HMUTEX lock(HMUTEX hm) {
		HMUTEX r = 0;
		if(mpi_mutex)
			r = mpi_mutex->lock(hm);
		return r;
	}

	void unlock(HMUTEX hm) {
		if(mpi_mutex)
			mpi_mutex->unlock(hm);
	}

	void add_listener(_log_listener_t *lstr) {
	}

	void remove_listener(_log_listener_t *lstr) {
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

#include "respawn.h"
#include "iProcess.h"
#include "iMemory.h"
#include "iRepository.h"

class cProcess: public iProcess {
private:
	iLlist	*mpi_list;


	_proc_t *validate_handle(HPROCESS h) {
		_proc_t *r = NULL;
		HMUTEX hm = mpi_list->lock();

		if(mpi_list->sel(h, hm))
			r = (_proc_t *)h;

		mpi_list->unlock(hm);
		return r;
	}

public:
	BASE(cProcess, "cProcess", RF_ORIGINAL, 1,0,0);

	bool object_ctl(_u32 cmd, void *arg, ...) {
		bool r = false;

		switch(cmd) {
			case OCTL_INIT:
				if((mpi_list = (iLlist *)_gpi_repo_->object_by_iname(I_LLIST, RF_CLONE)))
					r = mpi_list->init(LL_VECTOR, 1);
				break;
			case OCTL_UNINIT:
				_gpi_repo_->object_release(mpi_list);
				break;
		}

		return r;
	}

	HPROCESS exec(_cstr_t path, _cstr_t argv[], _cstr_t env[]=NULL) {
		HPROCESS r = NULL;
		_proc_t pcxt;
		_s32 rexec = -1;

		if(env)
			rexec = proc_exec_ve(&pcxt, path, argv, env);
		else
			rexec = proc_exec_v(&pcxt, path, argv);

		if(rexec == 0)
			// success
			r = mpi_list->add(&pcxt, sizeof(_proc_t));

		return r;
	}

	HPROCESS exec(int (*pcb)(void *), void *udata) {
		HPROCESS r = NULL;
		_proc_t pcxt;

		if(proc_exec_cb(&pcxt, pcb, udata) != -1)
			r = mpi_list->add(&pcxt, sizeof(_proc_t));

		return r;
	}

	_s32 close(HPROCESS h, _s32 signal=SIGINT) {
		_s32 r = -1;
		_proc_t *pcxt = validate_handle(h);

		if(pcxt) {
			while((r = proc_status(pcxt)) == -1) {
				// still running
				proc_signal(pcxt, signal);
				proc_wait(pcxt);
			}

			// close stdin/stdout pipes
			proc_close_pipe(pcxt);

			HMUTEX hm = mpi_list->lock();

			if(mpi_list->sel(pcxt, hm))
				mpi_list->del(hm);

			mpi_list->unlock(hm);
		}

		return r;
	}

	_s32 read(HPROCESS h, _u8 *ptr, _u32 sz, _u32 timeout=0) {
		_s32 r = -1;
		_proc_t *pcxt = validate_handle(h);

		if(pcxt) {
			if(timeout)
				r = proc_read_ts(pcxt, ptr, sz, timeout);
			else
				r = proc_read(pcxt, ptr, sz);
		}

		return r;
	}

	_s32 write(HPROCESS h, _u8 *ptr, _u32 sz) {
		_s32 r = -1;
		_proc_t *pcxt = validate_handle(h);

		if(pcxt)
			r = proc_write(pcxt, ptr, sz);

		return r;
	}

	_s32 status(HPROCESS h) {
		_s32 r = -1;
		_proc_t *pcxt = validate_handle(h);

		if(pcxt)
			r = proc_status(pcxt);

		return r;
	}

	bool wait(HPROCESS h) {
		bool r = false;
		_proc_t *pcxt = validate_handle(h);

		if(pcxt)
			r = (proc_wait(pcxt) != -1) ? true : false;

		return r;
	}
};

static cProcess _g_process_;

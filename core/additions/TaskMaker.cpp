#include <string.h>
#include <pthread.h>
#include "iRepository.h"
#include "iTaskMaker.h"
#include "iMemory.h"

#define TS_RUNNING	(1<<0)
#define TS_STOPPED	(1<<1)

typedef struct {
	iBase 		*pi_base;
	_task_proc_t	*proc;
	pthread_t 	thread;
	void		*arg;
	void		*tmaker;
	_u8		state;
}_task_t;

static void *starter(_task_t *task);

class cTaskMaker: public iTaskMaker {
private:
	iLlist	*mpi_list;

	friend void *starter(_task_t *task);

	HTASK get_handle(void *p) {
		HTASK r = 0;

		if(mpi_list) {
			_u32 sz;
			HMUTEX hm = mpi_list->lock();
			_task_t *t = (_task_t *)mpi_list->first(&sz, hm);

			if(t) {
				do {
					if(p == t->pi_base || p == t->proc) {
						r = (HTASK)t;
						break;
					}
				} while((t = (_task_t *)mpi_list->next(&sz, hm)));
			}

			mpi_list->unlock(hm);
		}

		return r;
	}

	HTASK start_task(_task_t *task) {
		HTASK r = 0;

		if(mpi_list) {
			HMUTEX hm = mpi_list->lock();
			_task_t *t = (_task_t *)mpi_list->add(&task, sizeof(_task_t), hm);

			if((t)) {
				if(pthread_create(&t->thread, 0, (_task_proc_t *)starter, t) != ERR_NONE)
					mpi_list->del(hm);
				else
					r = t;
			}
			mpi_list->unlock(hm);
		}

		return r;
	}

	void remove_task(_task_t *task) {
		if(mpi_list) {
			HMUTEX hm = mpi_list->lock();
			if((mpi_list->sel(task, hm)))
				mpi_list->del(hm);
			mpi_list->unlock(hm);
		}
	}

public:
	BASE(cTaskMaker, "cTaskMaker", RF_ORIGINAL, 1,0,0);

	bool object_ctl(_u32 cmd, void *arg) {
		bool r = false;

		switch(cmd) {
			case OCTL_INIT: {
				iRepository *pi_repo = (iRepository*)arg;
				if((mpi_list = (iLlist*)pi_repo->object_by_iname(I_LLIST, RF_CLONE))) {
					mpi_list->init(LL_VECTOR, 1);
					r = true;
				}
			} break;
			case OCTL_UNINIT: {
				iRepository *pi_repo = (iRepository*)arg;
				pi_repo->object_release(mpi_list);
				r = true;
			} break;
		}

		return r;
	}

	HTASK start(iBase *pi_base, void *arg=0) {
		HTASK r = 0;

		if(!(r = get_handle(pi_base))) {
			_task_t task;

			memset(&task, 0, sizeof(_task_t));
			task.pi_base = pi_base;
			task.arg = arg;
			task.tmaker = this;
			r = start_task(&task);
		}

		return r;
	}

	HTASK start(_task_proc_t *proc, void *arg=0) {
		_task_t task;

		memset(&task, 0, sizeof(_task_t));
		task.proc = proc;
		task.arg = arg;
		task.tmaker = this;
		return start_task(&task);
	}

	HTASK handle(iBase *pi_base) {
		return get_handle((void *)pi_base);
	}

	HTASK handle(_task_proc_t *proc) {
		return get_handle((void *)proc);
	}

	_err_t stop(HTASK h) {
		_err_t r = 0;
		//...
		return r;
	}

	_err_t join(HTASK h) {
		_err_t r = 0;
		//...
		return r;
	}

	_err_t detach(HTASK h) {
		_err_t r = 0;
		//...
		return r;
	}

	_err_t set_name(HTASK h, _cstr_t name) {
		_err_t r = 0;
		//...
		return r;
	}

	_err_t get_name(HTASK h, _str_t name, _u32 len) {
		_err_t r = 0;
		//...
		return r;
	}
};

static void *starter(_task_t *task) {
	void *r = 0;

	task->state |= TS_RUNNING;
	task->state &= ~TS_STOPPED;

	if(task->pi_base)
		task->pi_base->object_ctl(OCTL_START, task->arg);
	else if(task->proc)
		r = task->proc(task->arg);

	task->state &= ~TS_RUNNING;
	task->state |= TS_STOPPED;

	cTaskMaker *tm = (cTaskMaker *)task->tmaker;
	if(tm)
		tm->remove_task(task);

	return r;
}

static cTaskMaker _g_task_maker_;

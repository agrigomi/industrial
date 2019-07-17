#include <string.h>
#include <unistd.h>
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

	_task_t *add_task(_task_t *task) {
		_task_t *r = 0;

		if(mpi_list) {
			HMUTEX hm = mpi_list->lock();
			r = (_task_t *)mpi_list->add(task, sizeof(_task_t), hm);
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

	HTASK start_task(_task_t *task) {
		HTASK r = 0;

		if(pthread_create(&task->thread, 0, (_task_proc_t *)starter, task) == ERR_NONE) {
			r = task;
			usleep(1);
		}

		return r;
	}

	bool stop_task(_task_t *task) {
		bool r = false;

		if(task->pi_base) {
			if((r = task->pi_base->object_ctl(OCTL_STOP, 0))) {
				_u32 n = 100;
				while(task->state & TS_RUNNING) {
					usleep(100);
					n--;
				}

				if(!n)
					r = false;
			}
		} else if(task->proc) {
			if(pthread_cancel(task->thread) == ERR_NONE) {
				remove_task(task);
				r = true;
			}
		}

		return r;
	}

	_task_t *validate_handle(HTASK h) {
		_task_t *r = 0;

		if(mpi_list) {
			HMUTEX hm = mpi_list->lock();
			if(mpi_list->sel((_task_t *)h, hm))
				r = (_task_t *)h;
			mpi_list->unlock(hm);
		}

		return r;
	}

public:
	BASE(cTaskMaker, "cTaskMaker", RF_ORIGINAL, 1,0,0);

	bool object_ctl(_u32 cmd, void *arg, ...) {
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

			_task_t *t = add_task(&task);
			r = start_task(t);
		} else {
			_task_t *task = (_task_t *)r;
			if(!(task->state & TS_RUNNING))
				start_task(task);
		}

		return r;
	}

	HTASK start(_task_proc_t *proc, void *arg=0) {
		_task_t task;

		memset(&task, 0, sizeof(_task_t));
		task.proc = proc;
		task.arg = arg;

		_task_t *t = add_task(&task);
		return start_task(t);
	}

	HTASK handle(iBase *pi_base) {
		return get_handle((void *)pi_base);
	}

	HTASK handle(_task_proc_t *proc) {
		return get_handle((void *)proc);
	}

	bool stop(HTASK h) {
		bool r = false;
		_task_t *task = validate_handle(h);

		if(task) {
			if(task->state & TS_RUNNING)
				r = stop_task(task);
			else {
				remove_task(task);
				r = true;
			}
		}

		return r;
	}

	_err_t join(HTASK h) {
		_err_t r = ERR_UNKNOWN;
		_task_t *task = validate_handle(h);

		if(task)
			r = pthread_join(task->thread, 0);

		return r;
	}

	_err_t detach(HTASK h) {
		_err_t r = ERR_UNKNOWN;
		_task_t *task = validate_handle(h);

		if(task)
			r = pthread_detach(task->thread);

		return r;
	}

	_err_t set_name(HTASK h, _cstr_t name) {
		_err_t r = ERR_UNKNOWN;
		_task_t *task = validate_handle(h);

		if(task)
			r = pthread_setname_np(task->thread, name);

		return r;
	}

	_err_t get_name(HTASK h, _str_t name, _u32 len) {
		_err_t r = ERR_UNKNOWN;
		_task_t *task = validate_handle(h);

		if(task)
			r = pthread_getname_np(task->thread, name, len);

		return r;
	}
};

static cTaskMaker _g_task_maker_;

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
	_g_task_maker_.remove_task(task);

	pthread_exit(0);

	return r;
}


#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include "iRepository.h"
#include "iTaskMaker.h"
#include "iMemory.h"

#define TS_RUNNING	(1<<0)
#define TS_IDLE		(1<<1)
#define TS_STOPPED	(1<<2)

typedef struct {
	iBase 		*pi_base;
	_task_proc_t	*proc;
	pthread_t 	thread;
	void		*arg;
	_u8		state;
	_char_t		name[20];
}_task_t;

static void *starter(_task_t *task);
typedef void *_thread_t(void *);

class cTaskMaker: public iTaskMaker {
private:
	iPool	*mpi_pool;

	friend void *starter(_task_t *task);

	HTASK get_handle(iBase *pi_base) {
		HTASK r = 0;

		if(mpi_pool) {
			typedef struct {
				cTaskMaker *p_tmaker;
				iBase *pi_base;
				HTASK res;
			}_enum_t;

			_enum_t e = {this, pi_base, NULL};

			mpi_pool->enum_busy([](void *data, _u32 size, void *udata)->_s32 {
				_s32 r = ENUM_NEXT;
				_enum_t *pe = (_enum_t *)udata;
				_task_t *p_task = (_task_t *)data;

				if(pe->pi_base == p_task->pi_base) {
					pe->res = p_task;
					r = ENUM_CANCEL;
				}

				return r;
			}, &e);

			r = e.res;
		}

		return r;
	}

	HTASK start_task(_task_t *task) {
		HTASK r = 0;

		if(pthread_create(&task->thread, 0, (_thread_t *)starter, task) == ERR_NONE) {
			r = task;
			usleep(1);
		}

		return r;
	}

	bool stop_task(_task_t *task) {
		bool r = false;

		if(!(task->state & TS_IDLE) && (task->state & TS_RUNNING)) {
			if(task->pi_base) {
				if((r = task->pi_base->object_ctl(OCTL_STOP, 0))) {
					_u32 n = 100;
					while(!(task->state & TS_IDLE)) {
						usleep(10000);
						n--;
					}

					if(!n)
						r = false;
				}
			} else if(task->proc) {
				_u32 tm=10;

				task->proc(TM_SIG_STOP, task->arg);
				while(tm && !(task->state & TS_IDLE)) {
					tm--;
					usleep(10000);
				}

				if(task->state & TS_IDLE)
					r = true;
			}
		} else
			r = true;

		return r;
	}

	void free_task(_task_t *task) {
		mpi_pool->free(task);
	}

	_task_t *validate_handle(HTASK h) {
		_task_t *r = 0;

		if(mpi_pool) {
			if(mpi_pool->verify(h))
				r = (_task_t *)h;
		}

		return r;
	}

	bool init_pool(void) {
		return mpi_pool->init(sizeof(_task_t), [](_u8 op, void *data, void *udata) {
			cTaskMaker *p_tmaker = (cTaskMaker *)udata;
			_task_t *p_task = (_task_t *)data;

			switch(op) {
				case POOL_OP_NEW:
					break;
				case POOL_OP_BUSY:
					break;
				case POOL_OP_FREE:
					break;
				case POOL_OP_DELETE:
					p_tmaker->stop_task(p_task);
					p_task->state &= ~TS_RUNNING;
					while(!(p_task->state & TS_STOPPED))
						usleep(10000);
					break;
			};
		}, this);
	}

public:
	BASE(cTaskMaker, "cTaskMaker", RF_ORIGINAL, 1,0,0);

	bool object_ctl(_u32 cmd, void *arg, ...) {
		bool r = false;

		switch(cmd) {
			case OCTL_INIT: {
				iRepository *pi_repo = (iRepository*)arg;
				if((mpi_pool = (iPool*)pi_repo->object_by_iname(I_POOL, RF_CLONE)))
					r = init_pool();
			} break;
			case OCTL_UNINIT: {
				iRepository *pi_repo = (iRepository*)arg;
				pi_repo->object_release(mpi_pool);
				r = true;
			} break;
		}

		return r;
	}

	HTASK start(iBase *pi_base, void *arg=0) {
		HTASK r = 0;

		if(!(r = get_handle(pi_base))) {
			_task_t *p_task = (_task_t *)mpi_pool->alloc();

			if(p_task) {
				p_task->arg = arg;
				p_task->pi_base = pi_base;
				if(!(p_task->state & TS_RUNNING))
					r = start_task(p_task);
				else
					r = p_task;
			}
		}

		return r;
	}

	HTASK start(_task_proc_t *proc, void *arg=0, _cstr_t name=0) {
		HTASK r = 0;

		_task_t *p_task = (_task_t *)mpi_pool->alloc();

		if(p_task) {
			p_task->arg = arg;
			p_task->proc = proc;
			if(name)
				strncpy(p_task->name, name, sizeof(p_task->name)-1);
			else
				snprintf(p_task->name, sizeof(p_task->name), "%p", proc);
			if(!(p_task->state & TS_RUNNING))
				r = start_task(p_task);
			else
				r = p_task;
		}

		return r;
	}

	HTASK handle(iBase *pi_base) {
		return get_handle(pi_base);
	}

	bool stop(HTASK h) {
		bool r = false;
		_task_t *task = validate_handle(h);

		if(task) {
			if(task->state & TS_RUNNING)
				r = stop_task(task);
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

	task->state = TS_RUNNING;

	while(task->state & TS_RUNNING) {
		if(task->pi_base) {
			_object_info_t oi;

			task->pi_base->object_info(&oi);
			task->state &= ~TS_IDLE;
			_g_task_maker_.set_name(task, oi.cname);
			task->pi_base->object_ctl(OCTL_START, task->arg);
		} else if(task->proc) {
			_g_task_maker_.set_name(task, task->name);
			task->state &= ~TS_IDLE;
			r = task->proc(TM_SIG_START, task->arg);
		}

		if(!(task->state & TS_IDLE)) {
			task->state |= TS_IDLE;
			task->pi_base = NULL;
			task->proc = NULL;
			_g_task_maker_.set_name(task, "idle");
			_g_task_maker_.free_task(task);
		}

		usleep(100000);
	}

	task->state = TS_STOPPED;

	pthread_exit(0);

	return r;
}


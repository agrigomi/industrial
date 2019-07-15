#include "iMemory.h"
#include "iRepository.h"

#define COL_FREE	0
#define COL_BUSY	1

class cPool: public iPool {
private:
	iLlist	*mpi_list;
	_u32 	m_data_size;
public:
	BASE(cPool, "cPool", RF_CLONE, 1,0,0);

	bool object_ctl(_u32 cmd, void *, ...) {
		bool r = false;

		switch(cmd) {
			case OCTL_INIT:
				m_data_size = 0;
				if((mpi_list = dynamic_cast<iLlist *>(_gpi_repo_->object_by_iname(I_LLIST, RF_CLONE|RF_NONOTIFY))))
					r = true;
				break;

			case OCTL_UNINIT:
				_gpi_repo_->object_release(mpi_list, false);
				r = true;
				break;
		}

		return r;
	}

	bool init(_u32 data_size, iHeap *pi_heap=0) {
		bool r = false;

		if(mpi_list) {
			m_data_size = data_size;
			r = mpi_list->init(LL_VECTOR, 2, pi_heap);
		}

		return r;
	}

	void *alloc(void) {
		void *r = NULL;
		_u32 sz = 0;
		HMUTEX hm = mpi_list->lock();

		mpi_list->col(COL_FREE, hm);
		if((r = mpi_list->first(&sz, hm)))
			mpi_list->mov(r, COL_BUSY, hm);
		else {
			mpi_list->col(COL_BUSY, hm);
			r = mpi_list->add(m_data_size, hm);
		}

		mpi_list->unlock(hm);

		return r;
	}

	void free(void *rec) {
		HMUTEX hm = mpi_list->lock();

		mpi_list->col(COL_BUSY, hm);
		if(mpi_list->sel(rec, hm))
			mpi_list->mov(rec, COL_FREE, hm);

		mpi_list->unlock(hm);
	}

	void clear(void) {
		HMUTEX hm = mpi_list->lock();

		mpi_list->col(COL_BUSY, hm);
		mpi_list->clr(hm);
		mpi_list->col(COL_FREE, hm);
		mpi_list->clr(hm);
		mpi_list->unlock(hm);
	}
};

static cPool _g_pool_;

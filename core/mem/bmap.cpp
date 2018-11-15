// buffer map
#include "iMemory.h"
#include "iRepository.h"

#define BFREE	0
#define BBUSY	1
#define BDIRTY	2

typedef struct __attribute__((packed)) {
	_u8	state;
	void	*ptr;
	void	*udata;
} _buffer_t;

class cBufferMap: public iBufferMap {
private:
	iLlist	*mpi_list;
	_u32	m_bsize;
	_buffer_io_t *m_pcb_bio; // I/O callback

public:
	BASE(cBufferMap, "cBufferMap", RF_CLONE, 1,0,0);

	bool object_ctl(_u32 cmd, void *arg, ...) {
		bool r = false;

		switch(cmd) {
			case OCTL_INIT: {
				m_bsize = 0;
				m_pcb_bio = 0;
				iRepository *pi_repo = (iRepository *)arg;

				mpi_list = dynamic_cast<iLlist *>(pi_repo->object_by_iname(I_LLIST, RF_CLONE));
				if(mpi_list) {
					mpi_list->init(LL_VECTOR, 3);
					r = true;
				}
			} break;
			case OCTL_UNINIT: {
				iRepository *pi_repo = (iRepository *)arg;

				uninit();
				pi_repo->object_release(mpi_list);
				r = true;
			} break;
		}

		return r;
	}

	void init(_u32 buffer_size, _buffer_io_t *pcb_bio) {
	}

	void uninit(void) {
	}

	HBUFFER alloc(void *udata=0) {
		HBUFFER r = 0;

		//...

		return r;
	}

	void free(HBUFFER hb) {
	}

	void *ptr(HBUFFER hb) {
		_buffer_t *b = (_buffer_t *)hb;
		return b->ptr;
	}

	void dirty(HBUFFER hb) {
	}

	void *get_udata(HBUFFER hb) {
		_buffer_t *b = (_buffer_t *)hb;
		return b->udata;
	}

	void set_udata(HBUFFER hb, void *udata) {
	}

	void reset(HBUFFER hb=0) {
	}

	void flush(HBUFFER hb=0) {
	}
};

static cBufferMap _g_buffer_map_;

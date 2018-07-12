#include "iRepository.h"
#include "iMemory.h"
#include "iHT.h"
#include "xml.h"

static void *mem_alloc(unsigned int);
static void mem_free(void *, unsigned int);

class cHT: public iHT {
private:
	iHeap *mpi_heap;

	friend void *mem_alloc(unsigned int);
	friend void mem_free(void *, unsigned int);
public:
	BASE(cHT, "cHT", RF_ORIGINAL, 1,0,0);

	bool object_ctl(_u32 cmd, void *arg, ...) {
		bool r = false;

		switch(cmd) {
			case OCTL_INIT: {
				iRepository *pi_repo = (iRepository *)arg;

				mpi_heap = (iHeap *)pi_repo->object_by_iname(I_HEAP, RF_ORIGINAL);
				if(mpi_heap) {
					r = true;
				}
			} break;
			case OCTL_UNINIT: {
				iRepository *pi_repo = (iRepository *)arg;

				pi_repo->object_release(mpi_heap);
				r = true;
			} break;
		}

		return r;
	}

	HTCONTEXT create_context(void) {
		return xml_create_context(mem_alloc, mem_free);
	}

	void destroy_context(HTCONTEXT hc) {
		xml_destroy_context((_xml_context_t *)hc);
	}
};

static cHT _g_hypertext_;

static void *mem_alloc(unsigned int sz) {
	void *r = NULL;

	if(_g_hypertext_.mpi_heap)
		r = _g_hypertext_.mpi_heap->alloc(sz);

	return r;
}

static void mem_free(void *ptr, unsigned int sz) {
	if(_g_hypertext_.mpi_heap)
		_g_hypertext_.mpi_heap->free(ptr, sz);
}

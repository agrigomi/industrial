#include "iRepository.h"
#include "iMemory.h"
#include "iHT.h"
#include "xml.h"

static void *_mem_alloc(unsigned int);
static void _mem_free(void *, unsigned int);

class cXML: public iXML {
private:
	iHeap *mpi_heap;

	friend void *_mem_alloc(unsigned int);
	friend void _mem_free(void *, unsigned int);
public:
	BASE(cXML, "cXML", RF_ORIGINAL, 1,0,0);

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
		return xml_create_context(_mem_alloc, _mem_free);
	}

	void destroy_context(HTCONTEXT hc) {
		xml_destroy_context((_xml_context_t *)hc);
	}

	bool parse(HTCONTEXT hc, _str_t xml_text, _ulong size) {
		bool r = false;

		if(xml_parse((_xml_context_t *)hc, (unsigned char*)xml_text, size) == XML_OK)
			r = true;

		return r;
	}

	HTTAG select(HTCONTEXT hc, _cstr_t xpath, HTTAG start_point, _u32 index) {
		return xml_select((_xml_context_t *)hc, xpath,
				(_ht_tag_t *)start_point, index);
	}
};

static cXML _g_xml_;

static void *_mem_alloc(unsigned int sz) {
	void *r = NULL;

	if(_g_xml_.mpi_heap)
		r = _g_xml_.mpi_heap->alloc(sz);

	return r;
}

static void _mem_free(void *ptr, unsigned int sz) {
	if(_g_xml_.mpi_heap)
		_g_xml_.mpi_heap->free(ptr, sz);
}


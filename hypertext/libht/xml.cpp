#include "iRepository.h"
#include "iMemory.h"
#include "iHT.h"
#include "xml.h"

static void *_mem_alloc(unsigned int, void *);
static void _mem_free(void *, unsigned int, void *);

class cXML: public iXML {
private:
	iHeap *mpi_heap;

	friend void *_mem_alloc(unsigned int, void *);
	friend void _mem_free(void *, unsigned int, void *);
public:
	BASE(cXML, "cXML", RF_ORIGINAL, 1,0,0);

	bool object_ctl(_u32 cmd, void *arg, ...) {
		bool r = false;

		switch(cmd) {
			case OCTL_INIT: {
				iRepository *pi_repo = (iRepository *)arg;

				mpi_heap = (iHeap *)pi_repo->object_by_iname(I_HEAP, RF_CLONE);
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
		return xml_create_context(_mem_alloc, _mem_free, this);
	}

	void destroy_context(HTCONTEXT hc) {
		xml_destroy_context((_xml_context_t *)hc);
	}

	bool parse(HTCONTEXT hc, _cstr_t xml_text, _ulong size) {
		bool r = false;

		if(xml_parse((_xml_context_t *)hc, (unsigned char*)xml_text, size) == XML_OK)
			r = true;

		return r;
	}

	HTTAG select(HTCONTEXT hc, _cstr_t xpath, HTTAG start_point, _u32 index) {
		return xml_select((_xml_context_t *)hc, xpath,
				(_ht_tag_t *)start_point, index);
	}

	_str_t parameter(HTCONTEXT hc, HTTAG ht, _cstr_t pname, _u32 *sz) {
		return (_str_t)xml_tag_parameter((_xml_context_t *)hc, (_ht_tag_t *)ht, pname, sz);
	}

	_str_t name(HTTAG ht, _u32 *sz) {
		return (_str_t)xml_tag_name((_ht_tag_t *)ht, sz);
	}

	_str_t content(HTTAG ht, _u32 *sz) {
		return (_str_t)xml_tag_content((_ht_tag_t *)ht, sz);
	}

	// Compare strins
	_s32 compare(HTCONTEXT hc, _str_t str1, _str_t str2, _u32 size) {
		_xml_context_t *p_xc = (_xml_context_t *)hc;

		return ht_compare(p_xc->p_htc, (unsigned char *)str1, (unsigned char *)str2, size);
	}
};

static cXML _g_xml_;

static void *_mem_alloc(unsigned int sz, void *udata) {
	void *r = NULL;

	if(_g_xml_.mpi_heap)
		r = _g_xml_.mpi_heap->alloc(sz);

	return r;
}

static void _mem_free(void *ptr, unsigned int sz, void *udata) {
	if(_g_xml_.mpi_heap)
		_g_xml_.mpi_heap->free(ptr, sz);
}


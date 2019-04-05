#include "iBase.h"
#include "iRepository.h"
#include "iMemory.h"
#include "iHT.h"
#include "json.h"


class cJSON: public iJSON {
private:
	iHeap *mpi_heap;
public:
	BASE(cJSON, "cJSON", RF_ORIGINAL, 1,0,0);

	bool object_ctl(_u32 cmd, void *arg, ...) {
		bool r = false;

		switch(cmd) {
			case OCTL_INIT:
				if((mpi_heap = (iHeap *)_gpi_repo_->object_by_iname(I_HEAP, RF_ORIGINAL)))
					r = true;
				break;
			case OCTL_UNINIT:
				_gpi_repo_->object_release(mpi_heap);
				r = true;
				break;
		}

		return r;
	}

	// Create parser context
	HTCONTEXT create_context(void) {
		_json_context_t *r = json_create_context([](_u32 size, void *udata)->void* {
			void *r = 0;
			cJSON *pobj = (cJSON *)udata;

			if(pobj->mpi_heap)
				r = pobj->mpi_heap->alloc(size);
			return r;
		}, [](void *ptr, _u32 size, void *udata) {
			cJSON *pobj = (cJSON *)udata;

			if(pobj->mpi_heap)
				pobj->mpi_heap->free(ptr, size);
		}, this);

		return r;
	}
	// destroy parser context
	void destroy_context(HTCONTEXT) {
		/*...*/
	}
	// Parse (indexing) hypertext content
	bool parse(HTCONTEXT htc, // parser context
			_cstr_t content, // Content of hypertext
			_ulong size // Size of hypertext content
			) {
		return (json_parse((_json_context_t *)htc, (const unsigned char *)content, size)) ? false : true;
	}
	// Compare strins (according content encoding)
	_s32 compare(HTCONTEXT htc,
			_str_t str1, // string 1
			_str_t str2, // string 2
			_u32 sz// size
			) {
		_json_context_t *p_jcxt = (_json_context_t *)htc;

		return ht_compare(p_jcxt->p_htc, (unsigned char *)str1, (unsigned char *)str2, sz);
	}
};

static cJSON _g_json_;

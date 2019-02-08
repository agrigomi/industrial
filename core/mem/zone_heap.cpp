#include <malloc.h>
#include <mutex>
#include "iMemory.h"
#include "zone.h"

class cZoneHeap: public iHeap {
private:
	_zone_context_t m_zone;
	std::mutex	m_mutex;

	void init_zone_context(void) {
		m_zone.user_data = this;
		m_zone.limit = ZONE_DEFAULT_LIMIT;
		m_zone.zones = NULL;

		m_zone.pf_page_alloc = [](_u32 num_pages, _u64 limit, void *udata)->void* {
			return ::malloc(num_pages * ZONE_PAGE_SIZE);
		};

		m_zone.pf_page_free = [](void *ptr, _u32 num_pages, void *udata) {
			::free(ptr);
		};

		m_zone.pf_mutex_lock = [](_u64 mutex_handle, void *udata)->_u64 {
			cZoneHeap *p = (cZoneHeap *)udata;

			p->m_mutex.lock();
			return 0;
		};

		m_zone.pf_mutex_unlock = [](_u64 mutex_handle, void *udata) {
			cZoneHeap *p = (cZoneHeap *)udata;

			p->m_mutex.unlock();
		};
	}
public:
	BASE(cZoneHeap, "cZoneHeap", RF_ORIGINAL | RF_CLONE, 1,0,0);

	bool object_ctl(_u32 cmd, void *arg, ...) {
		bool r = false;

		switch(cmd) {
			case OCTL_INIT:
				init_zone_context();
				if(zone_init(&m_zone) == 0)
					r = true;
				break;
			case OCTL_UNINIT:
				zone_destroy(&m_zone);
				r = true;
				break;
		}

		return r;
	}

	void *alloc(_u32 size) {
		return zone_alloc(&m_zone, size, ZONE_DEFAULT_LIMIT);
	}

	void free(void *ptr, _u32 size) {
		zone_free(&m_zone, ptr, size);
	}

	bool verify(void *ptr, _u32 size) {
		bool r = false;

		if(zone_verify(&m_zone, ptr, size) == 1)
			r = true;

		return r;
	}

	void status(_heap_status_t *p_hs) {
	}
};

static cZoneHeap _g_zone_heap_;

#include <string.h>
#include <unistd.h>
#include "iRepository.h"
#include "private.h"

#define HCOL_FREE	0
#define HCOL_BUSY	1

bool root::init(_cstr_t doc_root, _cstr_t cache_path,
		_cstr_t cache_key, _cstr_t cache_exclude_path,
		iHeap *pi_heap) {
	bool r = false;

	strncpy(m_root_path, doc_root, sizeof(m_root_path)-1);
	strncpy(m_cache_path, cache_path, sizeof(m_cache_path)-1);
	strncpy(m_cache_key, cache_key, sizeof(m_cache_key)-1);
	mpi_fs = NULL;
	mpi_fcache = NULL;
	m_enable = false;
	if(pi_heap)
		mpi_heap = pi_heap;
	else
		mpi_heap = dynamic_cast<iHeap *>(_gpi_repo_->object_by_iname(I_HEAP, RF_ORIGINAL));
	m_nocache = NULL;
	m_sz_nocache = 0;
	mpi_handle_list = dynamic_cast<iLlist *>(_gpi_repo_->object_by_iname(I_LLIST, RF_CLONE | RF_NONOTIFY));
	mpi_str = dynamic_cast<iStr *>(_gpi_repo_->object_by_iname(I_STR, RF_ORIGINAL));
	mpi_mutex = dynamic_cast<iMutex *>(_gpi_repo_->object_by_iname(I_MUTEX, RF_CLONE|RF_NONOTIFY));

	if(mpi_handle_list && mpi_str && mpi_heap && mpi_mutex) {
		cache_exclude(cache_exclude_path);
		r = mpi_handle_list->init(LL_VECTOR, 2, pi_heap);
	} else
		destroy();

	return r;
}

void root::object_release(iBase **ppi) {
	if(*ppi) {
		_gpi_repo_->object_release(*ppi, false);
		*ppi = NULL;
	}
}

void root::destroy(void) {
	stop();

	object_release((iBase **)&mpi_fs);
	object_release((iBase **)&mpi_fcache);
	object_release((iBase **)&mpi_handle_list);
	object_release((iBase **)&mpi_str);
	object_release((iBase **)&mpi_mutex);
}

_str_t root::realloc_nocache(_u32 sz) {
	_str_t r = NULL;
	_str_t old = m_nocache;
	_u32 sz_new = m_sz_nocache + sz;

	if((r = (_str_t)mpi_heap->alloc(sz_new))) {
		memset(r, 0, sz_new);
		if(old) {
			memcpy(r, old, m_sz_nocache);
			mpi_heap->free(old, m_sz_nocache);
		}

		m_sz_nocache = sz_new;
		m_nocache = r;
	}

	return r;
}

void root::cache_exclude(_cstr_t path) {
	if(path) {
		_u32 sz = strlen(path);
		_u32 sz_old = strlen(m_nocache);
		_cstr_t fmt;

		if(path[0] != '/' && path[sz-1] != '/')
			fmt = (m_nocache) ? ":/%s/" : "/%s/";
		else if(path[0] == '/' && path[sz-1] != '/')
			fmt = (m_nocache) ? ":%s/" : "%s/";
		else if(path[0] != '/' && path[sz-1] == '/')
			fmt = (m_nocache) ? ":/%s" : "/%s";
		else
			fmt = (m_nocache) ? ":%s" : "%s";

		HMUTEX hm = mpi_mutex->lock();
		if(realloc_nocache(sz + 4))
			snprintf(m_nocache + sz_old, m_sz_nocache - sz_old, fmt, path);
		mpi_mutex->unlock(hm);
	}
}

_u32 root::get_url_path(_cstr_t url) {
	_u32 r = strlen(url);

	while(r && url[r-1] != '/')
		r--;

	return r;
}

bool root::cacheable(_cstr_t path, _u32 len) {
	bool r = true;

	if(m_nocache && m_sz_nocache) {
		m_hlock = mpi_mutex->lock(m_hlock);

		for(_u32 i = 0, j = 0; i < m_sz_nocache; i++) {
			if(m_nocache[i] == ':' || m_nocache[i] == 0) {
				_u32 sz = i - j;

				if(sz && sz <= len) {
					if(memcmp(m_nocache + j, path, sz) == 0) {
						r = false;
						break;
					}
				}

				j = i + 1;
			}
		}

		mpi_mutex->unlock(m_hlock);
	}

	return r;
}

HDOCUMENT root::open(_cstr_t url) {
	HDOCUMENT r = NULL;

	if(m_enable && mpi_fs && mpi_fcache && mpi_handle_list) {
		_char_t doc[MAX_DOC_ROOT_PATH * 2]="";

		if((strlen(url) + strlen(m_root_path) < sizeof(doc)-1)) {
			_handle_t *ph = alloc_handle(0);

			if(ph) {
				_u32 path_len = get_url_path(url);
				bool use_cache = cacheable(url, path_len);

				snprintf(doc, sizeof(doc), "%s%s",
					m_root_path,
					(strcmp(url, "/") == 0) ? "/index.html" : url);

				ph->mime = resolve_mime_type(doc);

				if(use_cache) {
					if((ph->hfc = mpi_fcache->open(doc)))
						r = ph;
				} else if((ph->pi_fio = mpi_fs->open(doc, O_RDONLY)))
					r = ph;

				if(!r)
					// move to free column
					mpi_handle_list->mov(ph, HCOL_FREE);
			}
		}
	}

	return r;
}

_handle_t *root::get_busy_handle(HDOCUMENT hdoc, HMUTEX hlock) {
	_handle_t *r = NULL;
	_u32 sz = 0;
	HMUTEX hm = mpi_handle_list->lock(hlock);

	mpi_handle_list->col(HCOL_BUSY, hm);
	if(mpi_handle_list->sel(hdoc, hm))
		r = (_handle_t *)mpi_handle_list->current(&sz, hm);

	mpi_handle_list->unlock(hm);

	return r;
}

_handle_t *root::alloc_handle(HMUTEX hlock) {
	_handle_t *r = NULL;
	_u32 sz = 0;
	HMUTEX hm = mpi_handle_list->lock(hlock);

	mpi_handle_list->col(HCOL_FREE, hm);
	if(!(r = (_handle_t *)mpi_handle_list->first(&sz, hm))) {
		_handle_t handle;

		handle.hfc = NULL;
		handle.pi_fio = NULL;

		mpi_handle_list->col(HCOL_BUSY, hm);
		r = (_handle_t *)mpi_handle_list->add(&handle, sizeof(_handle_t), hm);
	} else
		mpi_handle_list->mov(r, HCOL_BUSY, hm);

	mpi_handle_list->unlock(hm);

	return r;
}

void *root::ptr(HDOCUMENT hdoc, _ulong *size) {
	void *r = NULL;

	if(mpi_handle_list) {
		_handle_t *ph = get_busy_handle(hdoc, 0);

		if(ph) {
			if(ph->hfc) // pointer from file cache
				r = mpi_fcache->ptr(ph->hfc, size);
			else if(ph->pi_fio) {
				r = ph->pi_fio->map(MPF_READ);
				*size = ph->pi_fio->size();
			}
		}
	}

	return r;
}

void root::close(HDOCUMENT hdoc) {
	if(mpi_handle_list) {
		_handle_t *ph = get_busy_handle(hdoc, 0);

		if(ph) {
			if(ph->hfc) {
				mpi_fcache->close(ph->hfc);
				ph->hfc = NULL;
			} else if(ph->pi_fio) {
				mpi_fs->close(ph->pi_fio);
				ph->pi_fio = NULL;
			}

			// release handle
			mpi_handle_list->mov(ph, HCOL_FREE);
		}
	}
}

time_t root::mtime(HDOCUMENT hdoc) {
	time_t r = 0;

	if(mpi_handle_list) {
		_handle_t *ph = get_busy_handle(hdoc, 0);

		if(ph) {
			if(ph->hfc && mpi_fcache)
				r = mpi_fcache->mtime(ph->hfc);
			else if(ph->pi_fio)
				r = ph->pi_fio->modify_time();
		}
	}

	return r;
}

_cstr_t root::mime(HDOCUMENT hdoc) {
	_cstr_t r = NULL;

	if(mpi_handle_list) {
		_handle_t *ph = get_busy_handle(hdoc, 0);

		if(ph)
			r = (ph->mime) ? ph->mime : "";
	}

	return r;
}

void root::stop(void) {
	if(m_enable) {
		_u32 sz = 0;
		_u32 t = 100;

		m_enable = false;

		while(mpi_handle_list && t--) {
			HMUTEX hm = mpi_handle_list->lock();

			mpi_handle_list->col(HCOL_BUSY, hm);
			void *rec = mpi_handle_list->first(&sz, hm);

			mpi_handle_list->unlock(hm);
			if(!rec)
				break;
			else
				usleep(10000);
		}

		if(!t && mpi_handle_list) {
			_handle_t *ph = NULL;

			do {
				HMUTEX hm = mpi_handle_list->lock();

				mpi_handle_list->col(HCOL_BUSY, hm);
				ph = (_handle_t *)mpi_handle_list->first(&sz, hm);

				mpi_handle_list->unlock(hm);

				if(ph)
					close(ph);
			} while(ph);
		}

		object_release((iBase **)&mpi_fcache);
		object_release((iBase **)&mpi_fs);
	}
}

void root::start(void) {
	if(!m_enable) {
		mpi_fs = dynamic_cast<iFS *>(_gpi_repo_->object_by_iname(I_FS, RF_ORIGINAL));
		mpi_fcache = dynamic_cast<iFileCache *>(_gpi_repo_->object_by_iname(I_FILE_CACHE, RF_CLONE | RF_NONOTIFY));

		if(mpi_fs && mpi_fcache)
			m_enable = mpi_fcache->init(m_cache_path, m_cache_key, mpi_heap);
	}
}

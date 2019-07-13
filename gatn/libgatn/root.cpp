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
	mpi_heap = pi_heap;
	mpi_nocache_map = dynamic_cast<iMap *>(_gpi_repo_->object_by_iname(I_MAP, RF_CLONE | RF_NONOTIFY));
	mpi_handle_list = dynamic_cast<iLlist *>(_gpi_repo_->object_by_iname(I_LLIST, RF_CLONE | RF_NONOTIFY));
	mpi_str = dynamic_cast<iStr *>(_gpi_repo_->object_by_iname(I_STR, RF_ORIGINAL));

	if(mpi_nocache_map && mpi_handle_list) {
		r &= mpi_nocache_map->init(15, pi_heap);
		r &= mpi_handle_list->init(LL_VECTOR, 2, pi_heap);
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
	object_release((iBase **)&mpi_nocache_map);
	object_release((iBase **)&mpi_handle_list);
	object_release((iBase **)&mpi_str);
}

void root::cache_exclude(_cstr_t path) {
	_char_t lb[MAX_DOC_ROOT_PATH * 2]="";
	_char_t fldr[MAX_DOC_ROOT_PATH]="";

	if(mpi_str) {
		strncpy(lb, path, sizeof(lb)-1);

		while(mpi_str->div_str(lb, fldr, sizeof(fldr), lb, sizeof(lb), ":"))
			_cache_exclude(fldr, sizeof(fldr));

		if(strlen(fldr))
			_cache_exclude(fldr, sizeof(fldr));
	}
}

void root::_cache_exclude(_str_t path, _u32 sz) {
	_u32 l = strlen(path);

	if(l) {
		if(path[l-1] != '/') {
			// the path must be always terminated with '/'
			strncat(path, "/", sz - l);
			l++;
		}

		mpi_nocache_map->add(path, l, path, l);
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

	if(mpi_nocache_map) {
		_u32 sz=0;
		_map_enum_t map_enum = mpi_nocache_map->enum_open();
		_cstr_t str = (_cstr_t)mpi_nocache_map->enum_first(map_enum, &sz);

		while(str) {
			if(sz <= len && memcmp(path, str, sz) == 0) {
				r = false;
				break;
			}
			str = (_cstr_t)mpi_nocache_map->enum_next(map_enum, &sz);
		}

		mpi_nocache_map->enum_close(map_enum);
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
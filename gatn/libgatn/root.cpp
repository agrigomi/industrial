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
	mpi_fs = dynamic_cast<iFS *>(_gpi_repo_->object_by_iname(I_FS, RF_ORIGINAL));
	mpi_fcache = dynamic_cast<iFileCache *>(_gpi_repo_->object_by_iname(I_FILE_CACHE, RF_CLONE | RF_NONOTIFY));
	mpi_nocache_map = dynamic_cast<iMap *>(_gpi_repo_->object_by_iname(I_MAP, RF_CLONE | RF_NONOTIFY));
	mpi_handle_list = dynamic_cast<iLlist *>(_gpi_repo_->object_by_iname(I_LLIST, RF_CLONE | RF_NONOTIFY));
	mpi_str = dynamic_cast<iStr *>(_gpi_repo_->object_by_iname(I_STR, RF_ORIGINAL));

	if(mpi_fs && mpi_fcache && mpi_nocache_map && mpi_handle_list) {
		r = mpi_fcache->init(cache_path, cache_key, pi_heap);
		r &= mpi_nocache_map->init(15, pi_heap);
		r &= mpi_handle_list->init(LL_VECTOR, 2, pi_heap);

		if((m_enable = r))
			cache_exclude(cache_exclude_path);
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
	_char_t lb[MAX_DOC_ROOT_PATH]="";
	_char_t fldr[MAX_DOC_ROOT_PATH]="";

	if(mpi_str) {
		strncpy(lb, path, sizeof(lb)-1);

		while(mpi_str->div_str(lb, fldr, sizeof(fldr), lb, sizeof(lb), ":"))
			_cache_exclude(fldr);

		if(strlen(fldr))
			_cache_exclude(fldr);
	}
}

void root::_cache_exclude(_cstr_t path) {
	_u32 l = strlen(path);

	if(path[l] == '/')
		l--;

	mpi_nocache_map->add(path, l, path, l);
}

HDOCUMENT root::open(_cstr_t url) {
	HDOCUMENT r = NULL;

	if(m_enable) {
		//...
	}

	return r;
}

void root::close(HDOCUMENT hdoc) {
	//...
}

void root::stop(void) {
	if(m_enable) {
		_u32 sz = 0;

		m_enable = false;

		while(mpi_handle_list) {
			HMUTEX hm = mpi_handle_list->lock();

			mpi_handle_list->col(HCOL_BUSY, hm);
			void *rec = mpi_handle_list->first(&sz, hm);

			mpi_handle_list->unlock(hm);
			if(!rec)
				break;
			else
				usleep(10000);
		}
	}
}

void root::start(void) {
	if(!m_enable) {
		//...
		m_enable = true;
	}
}

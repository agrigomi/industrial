#include "iRepository.h"
#include "private.h"

#define HCOL_FREE	0
#define HCOL_BUSY	1

bool root::init(_cstr_t doc_root, _cstr_t cache_path,
		_cstr_t cache_key, _cstr_t cache_exclude,
		iHeap *pi_heap) {
	bool r = false;

	strncpy(m_root_path, doc_root, sizeof(m_root_path)-1);
	mpi_fcache = dynamic_cast<iFileCache *>(_gpi_repo_->object_by_iname(I_FILE_CACHE, RF_CLONE | RF_NONOTIFY));
	mpi_nocache_map = dynamic_cast<iMap *>(_gpi_repo_->object_by_iname(I_MAP, RF_CLONE | RF_NONOTIFY));
	mpi_handle_list = dynamic_cast<iLlist *>(_gpi_repo_->object_by_iname(I_LLIST, RF_CLONE | RF_NONOTIFY));
	mpi_str = dynamic_cast<iStr *>(_gpi_repo_->object_by_iname(I_STR, RF_ORIGINAL));

	if(mpi_fcache && mpi_nocache_map && mpi_handle_list) {
		r = mpi_fcache->init(cache_path, cache_key);
		r &= mpi_nocache_map->init(15, pi_heap);
		if(r)
			mpi_handle_list->init(LL_VECTOR, 2, pi_heap);

		parse_nocache_list(cache_exclude);
		m_enable = r;
	} else
		destroy();

	return r;
}

void root::destroy(void) {
	if(mpi_fcache) {
		_gpi_repo_->object_release(mpi_fcache, false);
		mpi_fcache = NULL;
	}

	if(mpi_nocache_map) {
		_gpi_repo_->object_release(mpi_nocache_map, false);
		mpi_nocache_map = NULL;
	}

	if(mpi_handle_list) {
		_gpi_repo_->object_release(mpi_handle_list, false);
		mpi_handle_list = NULL;
	}

	_gpi_repo_->object_release(mpi_str);

	m_enable = false;
}

void root::parse_nocache_list(_cstr_t nocache) {
	_char_t lb[MAX_DOC_ROOT_PATH]="";
	_char_t fldr[MAX_DOC_ROOT_PATH]="";

	if(mpi_str) {
		strncpy(lb, nocache, sizeof(lb)-1);

		while(mpi_str->div_str(lb, fldr, sizeof(fldr), lb, sizeof(lb), ":"))
			cache_exclude(fldr);

		if(strlen(fldr))
			cache_exclude(fldr);
	}
}

bool root::cache_exclude(_cstr_t path) {
	bool r = false;
	_u32 l = strlen(path);

	if(path[l] == '/')
		l--;

	if(mpi_nocache_map->add(path, l, path, l))
		r = true;

	return r;
}

HDOCUMENT root::open(_cstr_t url) {
	HDOCUMENT r = NULL;

	//...

	return r;
}

void root::close(HDOCUMENT hdoc) {
	//...
}

void root::stop(void) {
	//...
}

void root::start(void) {
	//...
}

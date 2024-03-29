#include <stdio.h>
#include <string.h>
#include <openssl/sha.h>
#include "iFS.h"
#include "iStr.h"
#include "iRepository.h"

#define USE_MUTEX
//#define USE_SPINLOCK

#ifdef USE_MUTEX
#include <mutex>
#endif

#ifdef USE_SPINLOCK
#include "spinlock.h"
#endif

typedef struct { // file cache entry
	iFileIO		*pi_fio; // file IO
#ifdef USE_MUTEX
	std::mutex 	sync;	// native mutex
#endif
#ifdef USE_SPINLOCK
	_spinlock_t	sync;
#endif
	_u32		refc; // reference counter
	_u32		acct; // access counter
	_char_t		sha_fname[SHA_DIGEST_LENGTH*2+1]; // sha1 of file path
	void		*ptr; // pointer to file content
	_ulong		size; // file size
	time_t		mtime; // last modification time of original file
	bool		remove;

	void clear(void) {
		pi_fio = NULL;
		ptr = NULL;
		refc = acct = 0;
		size = 0;
		mtime = 0;
		remove = false;
	}
}_fce_t;

#define MAX_FCACHE_PATH	512

class cFileCache: public iFileCache {
private:
	iMap	*mpi_map;
	iFS	*mpi_fs;
	iStr	*mpi_str;

	_char_t	m_cache_path[MAX_FCACHE_PATH];

	void hash(_cstr_t path, _char_t sha_out[SHA_DIGEST_LENGTH*2+1]) {
		SHA_CTX ctx;
		_uchar_t hb[SHA_DIGEST_LENGTH];

		SHA1_Init(&ctx);
		SHA1_Update(&ctx, path, strlen(path));
		SHA1_Final(hb, &ctx);

		for(_u32 i = 0, j = 0; i < SHA_DIGEST_LENGTH; i++)
			j += sprintf((char *)(sha_out+j), "%02X", hb[i]);
	}

	bool update_cache(_cstr_t path, _fce_t *pfce, time_t mtime=0) {
		bool r = false;

		if(pfce->pi_fio) {
			mpi_fs->close(pfce->pi_fio);
			pfce->pi_fio = NULL;
			pfce->ptr = NULL;
		}

		_char_t cache_path[MAX_FCACHE_PATH*2]="";
		bool _r = false;
		time_t _mtime = (mtime) ? mtime : mpi_fs->modify_time(path);

		snprintf(cache_path, sizeof(cache_path), "%s/%s", m_cache_path, pfce->sha_fname);

		if(pfce->mtime < _mtime)
			_r = mpi_fs->copy(path, (_cstr_t)cache_path);
		else
			_r = true;

		if(_r) {
			if((pfce->pi_fio = mpi_fs->open(cache_path))) {
				pfce->refc = 0;
				pfce->acct = 0;
				pfce->ptr = NULL;
				pfce->size = pfce->pi_fio->size();
				pfce->mtime = _mtime;
				pfce->remove = false;
				r = true;
			}
		}

		return r;
	}

	_fce_t *add_to_map(_cstr_t path, time_t mtime=0) {
		_fce_t *r = 0;

		if(mpi_map) {
			_fce_t fce;
			_u32 sz;

			fce.clear();
			hash(path, fce.sha_fname); // make cache file name

			HMUTEX hm = mpi_map->lock();

			if(!(r = (_fce_t *)mpi_map->get(fce.sha_fname, strlen(fce.sha_fname), &sz, hm))) {
				// create new entry
				if(update_cache(path, &fce, mtime))
					r = (_fce_t *)mpi_map->add(fce.sha_fname, strlen(fce.sha_fname), &fce, sizeof(_fce_t), hm);
			}

			mpi_map->unlock(hm);
		}

		return r;
	}

	void remove_cache_file(_fce_t *pfce) {
		_char_t cache_path[MAX_FCACHE_PATH*2]="";

		snprintf(cache_path, sizeof(cache_path), "%s/%s", m_cache_path, pfce->sha_fname);
		mpi_fs->remove(cache_path);
	}

	void remove_cache(_fce_t *pfce) {
		if(pfce->remove && pfce->refc == 0) {
			HMUTEX hm = mpi_map->lock();

			pfce->sync.lock();
			if(pfce->pi_fio) {
				if(pfce->ptr) {
					pfce->pi_fio->unmap();
					pfce->ptr = NULL;
				}
				mpi_fs->close(pfce->pi_fio);
				pfce->pi_fio = NULL;
			}
			pfce->sync.unlock();
			mpi_map->del(pfce->sha_fname, strlen(pfce->sha_fname), hm);
			remove_cache_file(pfce);

			mpi_map->unlock(hm);
		}
	}

	void close_cache(void) {
		if(mpi_map) {
			HMUTEX hm = mpi_map->lock();
			_map_enum_t me = mpi_map->enum_open();

			if(me) {
				_u32 sz = 0;
				_fce_t *pfce = (_fce_t *)mpi_map->enum_first(me, &sz, hm);

				while(pfce) {
					if(pfce->pi_fio) {
						mpi_fs->close(pfce->pi_fio);
						pfce->pi_fio = NULL;
					}
					pfce->ptr = NULL;
					pfce->size = 0;
					remove_cache_file(pfce);
					pfce = (_fce_t *)mpi_map->enum_next(me, &sz, hm);
				}

				mpi_map->enum_close(me);
			}

			mpi_map->unlock(hm);
		}
	}

public:
	BASE(cFileCache, "cFileCache", RF_CLONE, 1,0,0);

	bool object_ctl(_u32 cmd, void *arg, ...) {
		bool r = false;

		switch(cmd) {
			case OCTL_INIT: {
				iRepository *pi_repo = (iRepository *)arg;

				mpi_map = NULL;
				mpi_str = (iStr *)pi_repo->object_by_iname(I_STR, RF_ORIGINAL);
				mpi_fs = (iFS *)pi_repo->object_by_iname(I_FS, RF_ORIGINAL);

				if(mpi_str && mpi_fs)
					r = true;
			} break;
			case OCTL_UNINIT: {
				iRepository *pi_repo = (iRepository *)arg;

				close_cache();
				mpi_fs->rm_dir(m_cache_path);
				pi_repo->object_release(mpi_map);
				pi_repo->object_release(mpi_fs);
				pi_repo->object_release(mpi_str);
				r = true;
			} break;
		}

		return r;
	}

	bool make_cache_dir(_cstr_t path, _cstr_t dir) {
		bool r = false;

		if((_u32)snprintf(m_cache_path, sizeof(m_cache_path), "%s/%s",
				path, dir) < sizeof(m_cache_path)) {
			if(!(r = mpi_fs->access(m_cache_path)))
				r = mpi_fs->mk_dir(m_cache_path);
		}

		return r;
	}

	bool init(_cstr_t path, _cstr_t key=NULL, iHeap *pi_heap=NULL) {
		bool r = false;

		if(!mpi_map) {
			if((mpi_map = (iMap *)_gpi_repo_->object_by_iname(I_MAP, RF_CLONE)))
				r = mpi_map->init(127, pi_heap);

			if(r)
				r = make_cache_dir(path, I_FILE_CACHE);

			if(r) {
				_char_t sbase[MAX_FCACHE_PATH]="";
				bool auto_key = false;

				snprintf(sbase, sizeof(sbase), "%s", m_cache_path);

				if(!key)
					auto_key = true;
				else if(strlen(key) == 0)
					auto_key = true;

				if(auto_key) {
					_char_t saddr[32]="";

					snprintf(saddr, sizeof(saddr), "%lu", (_ulong)this);
					r = make_cache_dir(sbase, saddr);
				} else
					r = make_cache_dir(sbase, key);
			}
		}

		return r;
	}

	HFCACHE open(_cstr_t path) {
		HFCACHE r = 0;
		time_t mtime = mpi_fs->modify_time(path);
		_fce_t *pfce = add_to_map(path, mtime);
		bool success = true;

		if(pfce) {
			pfce->sync.lock();
			if(mtime > pfce->mtime) {
				if(pfce->refc == 0)
					success = update_cache(path, pfce, mtime);
			}

			if(success) {
				if(!pfce->pi_fio)
					update_cache(path, pfce, mtime);

				if(pfce->pi_fio) {
					r = pfce;
					pfce->refc++;
					pfce->acct++;
				}
			}
			pfce->sync.unlock();
		}

		return r;
	}

	_ulong read(HFCACHE hfc, void *buffer, _ulong offset, _ulong size) {
		ulong r = 0;
		_fce_t *pfce = (_fce_t *)hfc;

		if(pfce->pi_fio) {
			if(pfce->ptr) {
				// use mapping
				_u8 *ptr = (_u8 *)pfce->ptr;
				_ulong sz = (ptr + pfce->size >= ptr + offset + size) ?
						size : pfce->size - (ptr + offset - ptr);

				mpi_str->mem_cpy(buffer, ptr + offset, sz);
				r = sz;
			} else {
				pfce->pi_fio->seek(offset);
				r = pfce->pi_fio->read(buffer, size);
			}
		}

		return r;
	}

	void *ptr(HFCACHE hfc, _ulong *size) {
		void *r = 0;
		_fce_t *pfce = (_fce_t *)hfc;

		pfce->sync.lock();
		if(pfce->pi_fio) {
			if(!pfce->ptr)
				pfce->ptr = pfce->pi_fio->map(MPF_READ);
			*size = pfce->size;
			r = pfce->ptr;
		}
		pfce->sync.unlock();

		return r;
	}

	void close(HFCACHE hfc) {
		_fce_t *pfce = (_fce_t *)hfc;

		pfce->sync.lock();
		if(pfce->refc)
			pfce->refc--;
		if(!pfce->refc) {
			mpi_fs->close(pfce->pi_fio);
			pfce->pi_fio = NULL;
			pfce->ptr = NULL;
		}
		pfce->sync.unlock();
		remove_cache(pfce);
	}

	time_t mtime(HFCACHE hfc) {
		_fce_t *pfce = (_fce_t *)hfc;

		return pfce->mtime;
	}

	void remove(HFCACHE hfc) {
		_fce_t *pfce = (_fce_t *)hfc;

		pfce->remove = true;
		remove_cache(pfce);
	}
};

static cFileCache _g_file_cache_;

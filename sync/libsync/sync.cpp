#include <stdio.h>
#include <string.h>
#include "private.h"
#include "tString.h"
#include "tSTLVector.h"

typedef struct {
	tString src_fname;
	tString dst_fname;
	tString alias;
}_tosync_t;

static iFS *gpi_fs = NULL;
static iLog *gpi_log = NULL;
static tSTLVector<_tosync_t> gv_sync;

void sync_init(iFS *pi_fs, iLog *pi_log) {
	gpi_fs = pi_fs;
	gpi_log = pi_log;
}

static _cstr_t base_name(_cstr_t path) {
	_cstr_t r = path + strlen(path);

	while(r > path && *(r - 1) != '/')
		r--;

	return r;
}

void do_sync(void) {
	_cstr_t src = config_source();

	_gpi_repo_->extension_enum([](_cstr_t fname, _cstr_t alias, _base_entry_t *pbe,
					_u32 count, _u32 limit, void *udata) {
		if(strlen(fname)) {
			_cstr_t src_path = (_cstr_t)udata;
			_char_t src_file[1024] = "";
			_cstr_t file_base_name = base_name(fname);

			if(!config_exclude(file_base_name)) {
				time_t tm_src = 0, tm_dst = 0;

				snprintf(src_file, sizeof(src_file), "%s/%s", src_path, file_base_name);
				tm_src = gpi_fs->modify_time(src_file);
				tm_dst = gpi_fs->modify_time(fname);

				if(tm_src > tm_dst) {
					_tosync_t tmp;

					tmp.src_fname = src_file;
					tmp.dst_fname = fname;
					tmp.alias = alias;

					gv_sync.push_back(tmp);
				}
			}
		}
	}, (void *)src);

	_u32 n = gv_sync.size();

	for(_u32 i = 0; i < n; i++) {
		gpi_log->fwrite(LMT_INFO, "ExtSync: Unload '%s'", gv_sync[i].dst_fname.c_str());
		if(_gpi_repo_->extension_unload(gv_sync[i].alias.c_str()) == ERR_NONE) {
			gpi_log->fwrite(LMT_INFO, "ExtSync: Update '%s'", gv_sync[i].alias.c_str());
			gpi_fs->remove(gv_sync[i].dst_fname.c_str());
			gpi_log->fwrite(LMT_INFO, "ExtSync: Load '%s' as '%s'", gv_sync[i].dst_fname.c_str(),
					gv_sync[i].alias.c_str());
			gpi_fs->copy(gv_sync[i].src_fname.c_str(), gv_sync[i].dst_fname.c_str());
			_gpi_repo_->extension_load(base_name(gv_sync[i].dst_fname.c_str()),
						gv_sync[i].alias.c_str());
		} else
			gpi_log->fwrite(LMT_ERROR, "ExtSync: Failed to update '%s'", gv_sync[i].dst_fname.c_str());
	}

	gv_sync.clear();
}

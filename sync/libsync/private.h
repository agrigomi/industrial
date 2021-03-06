#ifndef __SYNC_PRIVATE_H__
#define __SYNC_PRIVATE_H__

#include "iFS.h"
#include "iHT.h"
#include "iLog.h"
#include "iExtSync.h"
#include "iRepository.h"

// config
void config_init(iFS *pi_fs, iJSON *pi_json,
		iLog *pi_log, _cstr_t config_fname);
bool config_touch(void);
bool config_load(void);
bool config_exclude(_cstr_t ext_fname);
_cstr_t config_source(void);

// sync
void sync_init(iFS *pi_fs, iLog *pi_log);
void do_sync(void);

#endif

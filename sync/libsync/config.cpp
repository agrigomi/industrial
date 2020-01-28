#include "private.h"
#include "tString.h"
#include "tSTLVector.h"

static iFS *gpi_fs = NULL;
static iJSON *gpi_json = NULL;
static iLog *gpi_log = NULL;
static _cstr_t g_config_fname = NULL;
static time_t g_config_mtime = 0;
static tString g_source;
static tSTLVector<tString> gv_exclude;

void config_init(iFS *pi_fs, iJSON *pi_json,
		iLog *pi_log, _cstr_t config_fname) {
	gpi_fs = pi_fs;
	gpi_json = pi_json;
	gpi_log = pi_log;
	g_config_fname = config_fname;
}


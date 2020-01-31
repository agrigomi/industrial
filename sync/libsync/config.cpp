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

bool config_touch(void) {
	bool r = false;
	time_t mtime = gpi_fs->modify_time(g_config_fname);

	if(mtime > g_config_mtime) {
		g_config_mtime = mtime;
		r = true;
	}

	return r;
}

static tString to_string(HTVALUE jv) {
	tString r = "";
	_u8 jvt = gpi_json->type(jv);

	if(jvt == JVT_STRING || jvt == JVT_NUMBER) {
		_u32 sz = 0;

		r.assign(gpi_json->data(jv, &sz));
	}

	return r;
}

static tString to_string(HTCONTEXT jcxt, _cstr_t jpath, HTVALUE jobj) {
	tString r = "";
	HTVALUE jv = gpi_json->select(jcxt, jpath, jobj);

	if(jv)
		r = to_string(jv);

	return r;
}

static void load_module(HTCONTEXT jcxt, HTVALUE jv_module) {
	if(gpi_json->type(jv_module) == JVT_OBJECT) {
		tString module = to_string(jcxt, "module", jv_module);
		tString alias = to_string(jcxt, "alias", jv_module);

		_gpi_repo_->extension_load(module.c_str(), alias.c_str());
	}
}

static void load_modules(HTCONTEXT jcxt) {
	HTVALUE jv_modules = gpi_json->select(jcxt, "load", NULL);

	if(jv_modules && gpi_json->type(jv_modules) == JVT_ARRAY) {
		_u32 idx = 0;
		HTVALUE jv_module = NULL;

		while((jv_module = gpi_json->by_index(jv_modules, idx)))
			load_module(jcxt, jv_module);
	}
}

static bool config_load_json(_u8 *p_content, _ulong sz_content) {
	bool r = false;
	HTCONTEXT jcxt = gpi_json->create_context();

	if(jcxt) {
		if((r = gpi_json->parse(jcxt, (_cstr_t)p_content, sz_content))) {
			load_modules(jcxt);
			//...
		}
		gpi_json->destroy_context(jcxt);
	}

	return r;
}

bool config_load(void) {
	bool r = false;
	iFileIO *pi_fio = gpi_fs->open(g_config_fname, O_RDONLY);

	if(pi_fio) {
		_ulong sz_content = pi_fio->size();
		_u8 *p_content = (_u8 *)pi_fio->map(MPF_READ);

		if(p_content && sz_content) {
			r = config_load_json(p_content, sz_content);
			pi_fio->unmap();
		}

		gpi_fs->close(pi_fio);
	}

	return r;
}

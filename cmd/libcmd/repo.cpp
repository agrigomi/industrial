#include <string.h>
#include <stdio.h>
#include "iCmd.h"
#include "iRepository.h"

// options
#define OPT_EXT_ONLY	"ext-only"
#define OPT_ALIAS	"alias"
#define OPT_EXT_DIR	"ext-dir"

// sctopns
#define ACT_LIST	"list"
#define ACT_LOAD	"load"
#define ACT_UNLOAD	"unload"

typedef struct {
	_cstr_t	a_name;
	_cmd_handler_t	*a_handler;
}_cmd_action_t;

class cCmdRepo;

static void fout(iIO *pi_io, _cstr_t fmt, ...) {
	if(pi_io) {
		va_list va;
		_char_t lb[1024]="";
		_u32 sz=0;

		va_start(va, fmt);
		sz = vsnprintf(lb, sizeof(lb), fmt, va);
		pi_io->write(lb, sz);
		va_end(va);
	}
}

static void flags2str(_u32 f, _str_t str, _u32 sz) {
	if(sz > 4) {
		_u8 i = 0;

		if(f & RF_ORIGINAL) {
			str[i] = 'O';
			i++;
		}
		if(f & RF_CLONE) {
			str[i] = 'C';
			i++;
		}
		if(f & RF_TASK) {
			str[i] = 'T';
			i++;
		}
		str[i] = 0;
	}
}

static void cmd_repo_list(iCmd *pi_cmd, iCmdHost *pi_cmd_host,
			iIO *pi_io, _cmd_opt_t *p_opt,
			_u32 argc, _str_t argv[]) {
	_enum_ext_t eext = _gpi_repo_->enum_ext_first();
	bool ext_only = pi_cmd_host->option_check(OPT_EXT_ONLY, p_opt);

	while(eext) {
		_u32 count = _gpi_repo_->enum_ext_array_count(eext);
		_u32 i = 0;

		fout(pi_io, "%s\tobjects:%d; limit:%d\n",
				_gpi_repo_->enum_ext_alias(eext),
				_gpi_repo_->enum_ext_array_count(eext),
				_gpi_repo_->enum_ext_array_limit(eext));
		if(!ext_only) {
			for(; i < count; i++) {
				_base_entry_t be;

				if(_gpi_repo_->enum_ext_array(eext, i, &be)) {
					_object_info_t oi;

					if(be.pi_base) {
						_char_t sf[10]="";
						be.pi_base->object_info(&oi);

						flags2str(oi.flags, sf, sizeof(sf));
						fout(pi_io, "\tiname:'%s'; cname:'%s'; flags:'%s'; version:%u.%u.%u; size:%u; ref:%u\n",
								oi.iname, oi.cname, sf,
								oi.version.major, oi.version.minor, oi.version.revision,
								oi.size, be.ref_cnt);
					}
				}
			}
		}
		eext = _gpi_repo_->enum_ext_next(eext);
	}
}

static void cmd_ext_load(iCmd *pi_cmd, iCmdHost *pi_cmd_host,
			iIO *pi_io, _cmd_opt_t *p_opt,
			_u32 argc, _str_t argv[]) {
	_str_t arg = 0;
	_str_t alias = pi_cmd_host->option_value(OPT_ALIAS, p_opt);
	_u32 i = 2;

	while((arg = pi_cmd_host->argument(argc, argv, p_opt, i))) {
		_err_t err = _gpi_repo_->extension_load(arg, alias);

		if(err != ERR_NONE)
			fout(pi_io, "Failed to load extension, error=%d\n", err);
		i++;
	}
}

static void cmd_ext_unload(iCmd *pi_cmd, iCmdHost *pi_cmd_host,
			iIO *pi_io, _cmd_opt_t *p_opt,
			_u32 argc, _str_t argv[]) {
	_str_t arg = 0;
	_u32 i = 2;

	while((arg = pi_cmd_host->argument(argc, argv, p_opt, i)))  {
		_err_t err = _gpi_repo_->extension_unload(arg);

		if(err != ERR_NONE)
			fout(pi_io, "Failed to unload extension, error=%d\n", err);

		i++;
	}
}

static _cmd_action_t _g_cmd_repo_actions_[]={
	{ ACT_LIST,		cmd_repo_list },
	{ ACT_LOAD,		cmd_ext_load },
	{ ACT_UNLOAD,		cmd_ext_unload },
	{ 0,			0 }
};

static void cmd_repo_handler(iCmd *pi_cmd, iCmdHost *pi_cmd_host,
			iIO *pi_io, _cmd_opt_t *p_opt,
			_u32 argc, _str_t argv[]) {
	_str_t arg = pi_cmd_host->argument(argc, argv, p_opt, 1);
	_str_t ext_dir = pi_cmd_host->option_value(OPT_EXT_DIR, p_opt);

	if(ext_dir)
		_gpi_repo_->extension_dir(ext_dir);

	if(arg) {
		_u32 n = 0;

		while(_g_cmd_repo_actions_[n].a_name) {
			if(strcmp(arg, _g_cmd_repo_actions_[n].a_name) == 0) {
				_g_cmd_repo_actions_[n].a_handler(pi_cmd, pi_cmd_host,
								pi_io, p_opt,
								argc, argv);
				break;
			}
			n++;
		}

		if(_g_cmd_repo_actions_[n].a_name == 0)
			fout(pi_io, "Unknown action '%s'\n", arg);
	}
}

static _cmd_opt_t _g_cmd_repo_opt_[]={
	{ OPT_EXT_ONLY,	OF_LONG,		0,	"Extensions only" },
	{ OPT_ALIAS,	OF_LONG|OF_VALUE,	0,	"Extension alias" },
	{ OPT_EXT_DIR,	OF_LONG|OF_VALUE,	0,	"Set extensions directory" },
	//...
	{ 0,		0,			0,	0 }
};

static _cmd_t _g_cmd_repo_[]={
	{ "repo",	_g_cmd_repo_opt_, cmd_repo_handler,
		"Repository management",
		"Manage reposiotory by following actions:\n"
		ACT_LIST "\t\t:Print available objects\n"
		ACT_LOAD "\t\t:Load extension\n"
		ACT_UNLOAD "\t\t:Unload extension\n",
		"repo [options] <action>"
	},
	{ 0,	0,	0,	0,	0,	0 }
};

class cCmdRepo: public iCmd {
public:
	BASE(cCmdRepo, "cCmdRepo", RF_ORIGINAL, 1,0,0);

	bool object_ctl(_u32 cmd, void *arg, ...) {
		bool r = false;

		switch(cmd) {
			case OCTL_INIT:
				r = true;
				break;
			case OCTL_UNINIT:
				r = true;
				break;
		}

		return r;
	}

	_cmd_t *get_info(void) {
		return _g_cmd_repo_;
	}
};

static cCmdRepo _g_cmd_repo_object_;


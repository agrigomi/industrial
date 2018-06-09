#include <string.h>
#include <stdio.h>
#include "iCmd.h"
#include "iRepository.h"

// options
#define OPT_EXT_ONLY	"ext-only"

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
	if(sz > 10) {
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

		fout(pi_io, "%s\n", _gpi_repo_->enum_ext_alias(eext));
		if(!ext_only) {
			for(; i < count; i++) {
				_base_entry_t be;

				if(_gpi_repo_->enum_ext_array(eext, i, &be)) {
					_object_info_t oi;

					if(be.pi_base) {
						_char_t sf[10]="";
						be.pi_base->object_info(&oi);

						flags2str(oi.flags, sf, sizeof(sf));
						fout(pi_io, "\t\tiname: '%s'; cname: '%s'; flags: '%s'; version: %u,%u,%u; size: %u\n",
								oi.iname, oi.cname, sf,
								oi.version.major, oi.version.minor, oi.version.revision,
								oi.size);
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
	//...
}

static void cmd_ext_unload(iCmd *pi_cmd, iCmdHost *pi_cmd_host,
			iIO *pi_io, _cmd_opt_t *p_opt,
			_u32 argc, _str_t argv[]) {
	//...
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
	}
}

static _cmd_opt_t _g_cmd_repo_opt_[]={
	{ OPT_EXT_ONLY,	OF_LONG,	0,	"Extensions only" },
	//...
	{ 0,		0,		0,	0 }
};

static _cmd_t _g_cmd_repo_[]={
	{ "repo",	_g_cmd_repo_opt_, cmd_repo_handler,
		"Repository management",
		"Manage reposiotory by following actions:\n"
		"\t" ACT_LIST "\t\t\tPrint available objects\n"
		"\t" ACT_LOAD "\t\t\tLoad extension\n"
		"\t" ACT_UNLOAD "\t\t\tUnload extension\n",
		"repo [options] <action>"
	},
	{ 0,	0,	0,	0,	0,	0 }
};

class cCmdRepo: public iCmd {
public:
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

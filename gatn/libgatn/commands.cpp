#include "iCmd.h"
#include "iGatn.h"
#include "iRepository.h"

static iGatn *gpi_gatn = NULL;

static iGatn *get_gatn(void) {
	if(!gpi_gatn)
		gpi_gatn = (iGatn *)_gpi_repo_->object_by_iname(I_GATN, RF_ORIGINAL);

	return gpi_gatn;
}

static void gatn_handler(iCmd *pi_cmd, // interface to command object
			iCmdHost *pi_cmd_host, // interface to command host
			iIO *pi_io, // interface to I/O object
			_cmd_opt_t *p_opt, // options array
			_u32 argc, // number of arguments
			_cstr_t argv[] // arguments
			) {
}

static _cmd_opt_t _g_opt_[] = {
	//...
	{ 0,				0,				0,	0 } // terminate options list
};

static _cmd_t _g_cmd_[] = {
	//...
	{ 0,	0,	0,	0,	0,	0} // terminate command list
};

class cGatnCmd: public iCmd {
public:
	BASE(cGatnCmd, "cGatnCmd", RF_ORIGINAL, 1,0,0);

	bool object_ctl(_u32 c, void *arg, ...) {
		bool r = false;

		switch(c) {
			case OCTL_INIT:
				r = true;
				break;
			case OCTL_UNINIT:
				if(gpi_gatn)
					_gpi_repo_->object_release(gpi_gatn);
				r = true;
		}
		return r;
	}

	_cmd_t *get_info(void) {
		return _g_cmd_;
	}
};

static cGatnCmd _g_gatn_cmd_;

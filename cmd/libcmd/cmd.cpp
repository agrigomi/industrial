#include "iCmd.h"
#include "iStr.h"
#include "iRepository.h"
#include "startup.h"

IMPLEMENT_BASE_ARRAY("libcmd", 10)

class cCmdHost: public iCmdHost {
private:
	iStr	*mpi_str;
public:
	BASE(cCmdHost, "cCmdHost", RF_ORIGINAL, 1,0,0);

	bool object_ctl(_u32 cmd, void *arg, ...) {
		bool r = false;

		switch(cmd) {
			case OCTL_INIT:
				if((mpi_str = (iStr *)_gpi_repo_->object_by_iname(I_STR, RF_ORIGINAL)))
					r = true;
				break;
			case OCTL_UNINIT:
				if(mpi_str)
					_gpi_repo_->object_release(mpi_str);
				r = true;
				break;
		}

		return r;
	}

	void exec(_str_t cmd_line, iIO *pi_io) {
		//...
	}
};

static cCmdHost _g_cmd_host_;

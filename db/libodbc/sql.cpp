#include "startup.h"
#include "iSQL.h"
#include "iRepository.h"
#include "private.h"

IMPLEMENT_BASE_ARRAY("libodbc", 10);

class cSQL: public iSQL {
private:
	HOBJECT	mhi_sql_pool;

public:
	BASE(cSQL, "cSQL", RF_ORIGINAL, 1,0,0);

	bool object_ctl(_u32 cmd, void *arg, ...) {
		bool r = false;

		switch(cmd) {
			case OCTL_INIT:
				if((mhi_sql_pool = _gpi_repo_->handle_by_iname(I_SQL_POOL)))
					r = true;
				break;
			case OCTL_UNINIT:
				//...
				break;
		}

		return r;
	}

	iSQLPool *create_sql_pool(_cstr_t connection_string) {
		iSQLPool *r = NULL;

		//...

		return r;
	}

	void destroy_sql_pool(iSQLPool *sql_pool) {
		//...
	}
};

static cSQL _g_sql_;

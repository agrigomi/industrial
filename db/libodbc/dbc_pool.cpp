#include <string.h>
#include "private.h"

#define CFREE		1
#define CPARTIAL	2
#define CFULL		3

#define MAX_CONNECTION_STRING	256

class cSQLPool: public iSQLPool {
private:
	_char_t m_connection_string[MAX_CONNECTION_STRING];
	iLlist	*mpi_list;

public:
	BASE(cSQLPool, "cSQLPool", RF_CLONE, 1,0,0);

	bool object_ctl(_u32 cmd, void *arg, ...) {
		bool r = false;

		switch(cmd) {
			case OCTL_INIT:
				if((mpi_list = (iLlist *)_gpi_repo_->object_by_iname(I_SQL_POOL, RF_CLONE)))
					r = mpi_list->init(LL_VECTOR, 3);
				break;
			case OCTL_UNINIT:
				if(mpi_list)
					_gpi_repo_->object_release(mpi_list);
				r = true;
				break;
		}

		return r;
	}

	bool init(_cstr_t connection_string) {
		bool r = false;

		strncpy(m_connection_string, connection_string, sizeof(m_connection_string)-1);
		//...

		return r;
	}

	_sql_t *alloc(void) {
		_sql_t *r = 0;

		//...

		return r;
	}

	void free(_sql_t *) {
		//...
	}
};

static cSQLPool _g_sql_pool_;

#include <string.h>
#include "private.h"

#define CFREE		0
#define CPARTIAL	1
#define CFULL		2

#define MAX_CONNECTION_STRING	256

class cSQLPool: public iSQLPool {
private:
	_char_t m_connection_string[MAX_CONNECTION_STRING];
	iLlist	*mpi_list;

	_dbc_t *new_connection(void) {
		_dbc_t *r = NULL;
		_dbc_t tmp;

		if(tmp.init(m_connection_string)) {
			HMUTEX hm = mpi_list->lock();

			mpi_list->col(CFREE, hm);
			r = (_dbc_t *)mpi_list->add(&tmp, sizeof(_dbc_t), hm);

			mpi_list->unlock(hm);
		}

		return r;
	}

	_dbc_t *get_connection(void) {
		_dbc_t *r = NULL;
		HMUTEX hm = mpi_list->lock();
		_u32 sz = 0;

		mpi_list->col(CPARTIAL, hm);
		if(!(r = (_dbc_t *)mpi_list->first(&sz, hm))) {
			mpi_list->col(CFREE, hm);
			r = (_dbc_t *)mpi_list->first(&sz, hm);
		}
		mpi_list->unlock(hm);

		return r;
	}

	void mov_connection(_dbc_t *pdbc) {
		SQLSMALLINT count = pdbc->count();
		SQLSMALLINT limit = pdbc->limit();
		HMUTEX hm = mpi_list->lock();

		if(count && count == limit)
			mpi_list->mov(pdbc, CFULL, hm);
		else if(count && count < limit)
			mpi_list->mov(pdbc, CPARTIAL, hm);
		else if(count == 0)
			mpi_list->mov(pdbc, CFREE, hm);

		mpi_list->unlock(hm);
	}

public:
	BASE(cSQLPool, "cSQLPool", RF_CLONE, 1,0,0);

	bool object_ctl(_u32 cmd, void *arg, ...) {
		bool r = false;

		switch(cmd) {
			case OCTL_INIT:
				if((mpi_list = (iLlist *)_gpi_repo_->object_by_iname(I_LLIST, RF_CLONE)))
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
		bool r = true;

		strncpy(m_connection_string, connection_string, sizeof(m_connection_string)-1);

		return r;
	}

	_sql_t *alloc(void) {
		_sql_t *r = 0;
		_dbc_t *pdbc = get_connection();

		if(!pdbc)
			pdbc = new_connection();

		if(pdbc)
			r = pdbc->alloc();

		if(r)
			mov_connection(pdbc);

		return r;
	}

	void free(_sql_t *psql) {
		sql *p_sql = (sql *)psql;
		_dbc_t *pdbc = p_sql->get_dbc();

		if(pdbc) {
			pdbc->free(p_sql);
			mov_connection(pdbc);
		}
	}
};

static cSQLPool _g_sql_pool_;

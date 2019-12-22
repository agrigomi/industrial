#include "private.h"

bool dbc::init(_cstr_t connect_string) {
	bool r = false;

	/* Allocate an environment handle */
	if(SQL_SUCCEEDED(SQLAllocHandle(SQL_HANDLE_ENV, SQL_NULL_HANDLE, &m_henv))) {
		/* We want ODBC 3 support */
		SQLSetEnvAttr(m_henv, SQL_ATTR_ODBC_VERSION, (void *) SQL_OV_ODBC3, 0);
		/* Allocate a connection handle */
		if(SQL_SUCCEEDED(SQLAllocHandle(SQL_HANDLE_DBC, m_henv, &m_hdbc))) {
			/* Connect to the ODBC driver */
			if(SQL_SUCCEEDED(SQLDriverConnect(m_hdbc, NULL, (SQLCHAR *)connect_string, SQL_NTS,
			                 0, 0, 0, SQL_DRIVER_COMPLETE))) {
				if((mpi_stmt_pool = (iPool *)_gpi_repo_->object_by_iname(I_POOL, RF_CLONE))) {
					r = mpi_stmt_pool->init(sizeof(_sql_t), [](_u8 op, void *data, void *udata) {
						switch(op) {
							case POOL_OP_NEW:
								break;
							case POOL_OP_BUSY:
								break;
							case POOL_OP_FREE:
								break;
							case POOL_OP_DELETE:
								break;
						}
					}, this);
				}
			}
		}
	}

	if(!r)
		destroy();

	return r;
}

void dbc::destroy(void) {
	if(m_hdbc) {
		/* Disconnect from driver */
		SQLDisconnect(m_hdbc);

		/* Deallocate connection handle */
		SQLFreeHandle(SQL_HANDLE_DBC, m_hdbc);
		m_hdbc = NULL;
	}

	if(m_henv) {
		/* Deallocate environment handle */
		SQLFreeHandle(SQL_HANDLE_ENV, m_henv);
		m_henv = NULL;
	}

	if(mpi_stmt_pool) {
		_gpi_repo_->object_release(mpi_stmt_pool);
		mpi_stmt_pool = 0;
	}
}

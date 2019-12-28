#include "private.h"

bool sql::_init(_dbc_t *pdbc) {
	bool r = false;

	mp_dbc = pdbc;

	SQLRETURN ret = SQLAllocHandle(SQL_HANDLE_STMT, mp_dbc->handle(), &m_hstmt);
	if(SQL_SUCCEEDED(ret))
		r = true;

	return r;
}

void sql::_free(void) {
	if(m_hstmt) {
		SQLFreeStmt(m_hstmt, SQL_UNBIND);
		SQLFreeStmt(m_hstmt, SQL_RESET_PARAMS);
		SQLFreeStmt(m_hstmt, SQL_CLOSE);
	}
}

void sql::_destroy(void) {
	_free();

	if(m_hstmt) {
		SQLFreeHandle(SQL_HANDLE_STMT, m_hstmt);
		m_hstmt = NULL;
	}
}


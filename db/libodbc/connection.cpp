#include "private.h"

bool dbc::init(_cstr_t connect_string) {
	bool r = false;

	/* Allocate an environment handle */
	if(SQL_SUCCEEDED(SQLAllocHandle(SQL_HANDLE_ENV, SQL_NULL_HANDLE, &m_henv))) {
		//...
	}

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
}

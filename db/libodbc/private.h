#ifndef __ODBC_PRIVATE_H__
#define __ODBC_PRIVATE_H__

#include <sqltypes.h>
#include <sql.h>
#include "iSQL.h"

struct sql: public _sql_t {
private:
	SQLHDBC		m_hdbc;
	SQLHSTMT	m_hstmt;

	bool _init(SQLHDBC hdbc);
	void _free(void);
	void _destroy(void);
	//...

public:
	sql() {
		m_hstmt = NULL;
	}
	//...
};


#endif

#ifndef __ODBC_PRIVATE_H__
#define __ODBC_PRIVATE_H__

#include <sqltypes.h>
#include <sql.h>
#include "iSQL.h"

struct sql: public _sql_t {
private:
	SQLHDBC	m_hdbc;

	bool _init(SQLHDBC hdbc);
	//...

public:
	//...
};


#endif

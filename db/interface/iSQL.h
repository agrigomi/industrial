#ifndef __I_SQL_H__
#define __I_SQL_H__

#include <sqltypes.h>
#include <sql.h>
#include <sqlext.h>
#include "iBase.h"

#define I_SQL		"iSQL"
#define I_SQL_POOL	"iSQLPool"

typedef struct {
	virtual void reset(void)=0;
	virtual bool prepare(_cstr_t)=0;
	virtual bool execute(_cstr_t query=NULL)=0;
	//...
}_sql_t;

class iSQLPool: public iBase {
public:
	INTERFACE(iSQLPool, I_SQL_POOL);

	virtual bool init(_cstr_t connection_string)=0;
	virtual _sql_t *alloc(void)=0;
	virtual void free(_sql_t *)=0;
};

class iSQL: public iBase {
public:
	INTERFACE(iSQL, I_SQL);

	virtual iSQLPool *create_sql_pool(_cstr_t connection_string)=0;
	virtual void destroy_sql_pool(iSQLPool *)=0;
};

#endif

#ifndef __I_SQL_H__
#define __I_SQL_H__

#define I_SQL_POOL	"iSQLPool"

#include "iBase.h"



class iSQLPool: public iBase {
public:
	INTERFACE(iSQLPool, I_SQL_POOL);
};

#endif

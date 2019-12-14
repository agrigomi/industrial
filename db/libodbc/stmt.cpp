#include "private.h"

bool sql::_init(SQLHDBC hdbc) {
	bool r = false;

	m_hdbc = hdbc;
	//...

	return r;
}


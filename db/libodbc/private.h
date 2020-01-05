#ifndef __ODBC_PRIVATE_H__
#define __ODBC_PRIVATE_H__

#include "iSQL.h"
#include "iMemory.h"
#include "iRepository.h"
#include "iLog.h"

typedef struct dbc _dbc_t;

struct sql: public _sql_t {
private:
	_dbc_t		*mp_dbc;
	SQLHSTMT	m_hstmt;
	SQLRETURN	m_ret;

public:
	sql() {
		m_hstmt = NULL;
	}

	_dbc_t *get_dbc(void) {
		return mp_dbc;
	}

	bool _init(_dbc_t *pdbc);
	void _free(void);
	void _destroy(void);
	void reset(void) {
		_free();
	}
	bool prepare(_cstr_t);
	bool execute(_cstr_t query=NULL);
	bool bind_params(_bind_param_t [], _u32 count);
};

struct dbc {
private:
	SQLHENV		m_henv;
	SQLHDBC		m_hdbc;
	SQLUSMALLINT	m_stmt_limit;
	SQLUSMALLINT	m_stmt_count;
	iPool		*mpi_stmt_pool;
	iLog		*mpi_log;
public:
	dbc() {
		m_henv = NULL;
		m_hdbc = NULL;
		m_stmt_limit = 0;
		m_stmt_count = 0;
		mpi_stmt_pool = NULL;
		mpi_log = NULL;
	}

	SQLSMALLINT limit(void) {
		return m_stmt_limit;
	}

	SQLSMALLINT count(void) {
		return m_stmt_count;
	}

	SQLHDBC handle(void) {
		return m_hdbc;
	}

	bool init(_cstr_t connect_string);
	void destroy(void);
	sql *alloc(void);
	void free(sql *);
};

#endif

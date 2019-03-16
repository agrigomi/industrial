#ifndef __I_REPOSITORY_H__
#define __I_REPOSITORY_H__

#include "iBase.h"
#include "err.h"

#define I_REPOSITORY	"iRepository"

#define ST_INITIALIZED	(1<<0)
#define ST_DISABLED	(1<<1)

typedef struct {
	iBase *pi_base;
	_u32  ref_cnt;
	_u8   state;
}_base_entry_t;

// ogject request flags
#define RQ_NAME		(1<<0)
#define RQ_INTERFACE	(1<<1)
#define RQ_VERSION	(1<<2)
#define RQ_CMP_OR	(1<<7) // comparsion type OR (AND is by default)

typedef struct {
	_u8 		flags;     // request flags
	_cstr_t		cname;     // class name
	_cstr_t		iname;     // interface name
	_version_t	version;
}_object_request_t;

typedef _base_entry_t* HOBJECT;
typedef void* HNOTIFY;
typedef void* _enum_ext_t;

#define SCAN_ORIGINAL	1
#define SCAN_CLONE	2

// notification flags
#define NF_LOAD		(1<<0)
#define NF_INIT		(1<<1)
#define NF_START	(1<<2)
#define NF_STOP		(1<<3)
#define NF_UNINIT	(1<<4)
#define NF_REMOVE	(1<<5)
#define NF_UNLOAD	(1<<6)

typedef struct {
	_u32	flags;
	HOBJECT	hobj;
	iBase	*object;
}_notification_t;

class iRepository: public iBase {
public:
	INTERFACE(iRepository, I_REPOSITORY);
	virtual iBase *object_request(_object_request_t *, _rf_t)=0;
	virtual void   object_release(iBase *, bool notify=true)=0;
	virtual iBase *object_by_cname(_cstr_t cname, _rf_t)=0;
	virtual iBase *object_by_iname(_cstr_t iname, _rf_t)=0;
	virtual iBase *object_by_handle(HOBJECT, _rf_t)=0;
	virtual HOBJECT handle_by_iname(_cstr_t iname)=0;
	virtual HOBJECT handle_by_cname(_cstr_t cname)=0;

	virtual void init_array(_base_entry_t *array, _u32 count)=0;

	// notifications
	virtual HNOTIFY monitoring_add(iBase *mon_obj, // monitored object
					_cstr_t mon_iname, // monitored interface
					_cstr_t mon_cname, // monitored object name
					iBase *handler_obj,// notifications receiver
					_u8 scan_flags=0 // scan for already registered objects
					)=0;
	virtual void monitoring_remove(HNOTIFY)=0;

	// extensions
	virtual void extension_dir(_cstr_t dir)=0;
	virtual _err_t extension_load(_cstr_t file, _cstr_t alias=0)=0;
	virtual _err_t extension_unload(_cstr_t alias)=0;
	// enumeration
	virtual _enum_ext_t enum_ext_first(void)=0;
	virtual _enum_ext_t enum_ext_next(_enum_ext_t)=0;
	virtual _cstr_t enum_ext_alias(_enum_ext_t)=0;
	virtual _u32 enum_ext_array_count(_enum_ext_t)=0;
	virtual _u32 enum_ext_array_limit(_enum_ext_t)=0;
	virtual bool enum_ext_array(_enum_ext_t, _u32 aidx, _base_entry_t*)=0;
	virtual void destroy(void)=0;
};

extern iRepository *_gpi_repo_;

#define BY_INAME(iname, flags) \
	_gpi_repo_->object_by_iname(iname, flags)
#define BY_CNAME(cname, flags) \
	_gpi_repo_->object_by_cname(cname, flags)

#endif


#ifndef __I_BASE_H__
#define __I_BASE_H__

#include "dtype.h"

#define I_BASE	"iBase"

#define INTERFACE(_class_, iname) \
	_class_() {} \
	virtual ~_class_() {} \
	virtual _cstr_t interface_name(void) 	{ return iname; }

typedef union {
	_u32	version;
	struct {
		_u32	revision:16;
		_u32	minor	:8;
		_u32	major	:8;
	};
}_version_t;

typedef _u8 	_rf_t;

// flags
#define RF_CLONE     (1<<0)
#define RF_ORIGINAL  (1<<1)
#define RF_TASK      (1<<2)
#define RF_NONOTIFY  (1<<3)

typedef struct {
	_cstr_t		iname;   // interface name
	_cstr_t		cname;   // class name
	_u32		size;    // object size
	_version_t	version; // object version
	_rf_t		flags;   // repository flags
}_object_info_t;

typedef struct link_info _link_info_t;

class iBase {
public:
	INTERFACE(iBase, I_BASE);
	virtual const _link_info_t *object_link(_u32 *cnt) {
		*cnt = 0;
		return 0;
	}
	virtual void object_info(_object_info_t *pi)=0;
	virtual bool object_ctl(_u32 cmd, void *arg, ...)=0;
};

// object controll
#define OCTL_INIT	10 // arg: iRepository*
#define OCTL_UNINIT	11 // arg: iRepository*
#define OCTL_START	12 // arg: void*
#define OCTL_STOP	13
#define OCTL_NOTIFY	14 // arg: _notification_t*

// reference controll
#define RCTL_REF	10
#define RCTL_UNREF	11

typedef void _ref_ctl_t(_u32, void*);

struct link_info {
	iBase		**ppi_base;
	_cstr_t 	iname;
	_cstr_t		cname;
	_rf_t		flags;
	_ref_ctl_t	*p_ref_ctl;
};

#define CONSTRUCTOR(_class_) \
	_class_()

#define INFO(_class_, name, f, a, i, r) \
	virtual void object_info(_object_info_t *pi) { \
		pi->iname = interface_name(); \
		pi->cname = name; \
		pi->flags = f; \
		pi->size = sizeof(_class_); \
		pi->version.major = a; \
		pi->version.minor = i; \
		pi->version.revision = r; \
	}

#define BEGIN_LINK_MAP const _link_info_t *object_link(_u32 *cnt) {\
	static const _link_info_t _link_map_[]= {
#define END_LINK_MAP };\
		 *cnt = sizeof(_link_map_) / sizeof(_link_info_t);\
		 return _link_map_;}

#define BASE_REF(x) (iBase**)&x

#define DESTRUCTOR(_class_) \
	virtual ~_class_()

#define BASE(_class_, name, flags, a, i, r) \
	CONSTRUCTOR(_class_) { \
		register_object(dynamic_cast<iBase *>(this)); \
	} \
	DESTRUCTOR(_class_) {} \
	INFO(_class_, name, flags, a, i, r)


extern void register_object(iBase *);

#endif


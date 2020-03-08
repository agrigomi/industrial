#ifndef __I_BASE_H__
#define __I_BASE_H__

#include "dtype.h"

#define I_BASE	"iBase"

/**
 * Interface macro.
 *
 * Implements constructor, destructor and interface_name function.
 *
 * @param _class_ Class name.
 * @param iname Interface name.
 */
#define INTERFACE(_class_, iname) \
	_class_() {} \
	virtual ~_class_() {} \
	/**
	 * Interface name virtual method.
	 *
	 * Only used for return the interface name.
	 */
	virtual _cstr_t interface_name(void) 	{ return iname; }

/**
 * Version structure
 *
 * Contains all components of version number
 * major, minor and revision
 */
typedef union {
	/** Packed version number
	 * Contains revision, minor and majot parts of version.
	 */
	_u32	version;
	struct {
		_u32	revision:16;
		_u32	minor	:8;
		_u32	major	:8;
	};
}_version_t;

/**
 * Predefined type (one byte for repository flags.
 */
typedef _u8 	_rf_t;

// flags
#define RF_CLONE     	(1<<0)
#define RF_ORIGINAL  	(1<<1)
#define RF_TASK      	(1<<2)
#define RF_NONOTIFY  	(1<<3)
#define RF_NOCRITICAL	(1<<4)
#define RF_KEEP_PENDING	(1<<5)
#define RF_POST_INIT	(1<<6)

#define RF_PLUGIN	RF_NOCRITICAL | RF_KEEP_PENDING | RF_POST_INIT

/**
 * Object properties.
 *
 * It represents the object parameters.
 */
typedef struct {
	_cstr_t		iname;   //!< interface name
	_cstr_t		cname;   //!< class name
	_u32		size;    //!< object size
	_version_t	version; //!< object version
	_rf_t		flags;   //!< repository flags
}_object_info_t;

typedef struct link_info _link_info_t;

/**
 * Base interface for all components.
 *
 * Every component must inherit by this interface.
 * This is a virtual base class that contains a several simple methods.
 */
class iBase {
public:
	INTERFACE(iBase, I_BASE);
	/**
	 * Returns pointer to link structure.
	 *
	 * @param cnt [out]  In this parameter, function returns
	 *                   the number of _link_info_t structures.
	 * @return Ponter to link map (array of _link_info_t structures)
	 */
	virtual const _link_info_t *object_link(_u32 *cnt) {
		*cnt = 0;
		return 0;
	}
	/**
	 * Access to object information.
	 * @param pi [out] Pointer to _object_info_t structure.
	 */
	virtual void object_info(_object_info_t *pi)=0;
	/**
	 * Object controll.
	 *
	 * The repository uses this function to initialize object with cmd = OCTL_INIT,
	 *  destroy with cmd = OCTL_UNINIT,
	 *  start task with cmd = OCTL_START and stop task with cmd = OCTL_STOP.
	 * @param cmd Command number.
	 * @param arg Pointer to first argument.
	 * @return true/false
	 */
	virtual bool object_ctl(_u32 cmd, void *arg, ...)=0;
};

// object controll
#define OCTL_INIT	10 // arg: iRepository*
#define OCTL_UNINIT	11 // arg: iRepository*
#define OCTL_START	12 // arg: void*
#define OCTL_STOP	13
#define OCTL_NOTIFY	14 // arg: _notification_t*

// reference controll
#define RCTL_LOAD	10
#define RCTL_REF	11
#define RCTL_UNREF	12
#define RCTL_UNLOAD	13

typedef void _ref_ctl_t(_u32, void*);
typedef void _info_ctl_t(_u32, _object_info_t *, void *);

struct link_info {
	iBase		**ppi_base;
	_cstr_t 	iname;
	_cstr_t		cname;
	_rf_t		flags;
	_ref_ctl_t	*p_ref_ctl;
	_info_ctl_t	*p_info_ctl;
	void		*udata;
};

#define CONSTRUCTOR(_class_) \
	_class_()

#define OBJECT_INFO(_class_, name, f, a, i, r) \
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
#define LINK(p, i, c, f, r, u) \
	{ BASE_REF(p), i, c, f, r, NULL, u }
#define INFO(i, c, r, u) \
	{ NULL, i, c, RF_PLUGIN, NULL, r, u}

#define DESTRUCTOR(_class_) \
	virtual ~_class_()

#define BASE(_class_, name, flags, a, i, r) \
	CONSTRUCTOR(_class_) { \
		register_object(dynamic_cast<iBase *>(this)); \
	} \
	DESTRUCTOR(_class_) {} \
	OBJECT_INFO(_class_, name, flags, a, i, r)


extern void register_object(iBase *);

#endif


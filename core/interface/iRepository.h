#ifndef __I_REPOSITORY_H__
#define __I_REPOSITORY_H__

#include "iBase.h"
#include "err.h"

#define I_REPOSITORY	"iRepository"

#define ST_INITIALIZED	(1<<0)
#define ST_DISABLED	(1<<1)
#define ST_PENDING	(1<<2)

typedef _u8 _cstat_t;


/**
 * Runtime object information.  */
typedef struct {
	iBase    *pi_base; //!< Pointer to iBase
	_u32  	 ref_cnt; //!< Reference counter
	_cstat_t state; //!< Current state (ST_INITIALIZED, ST_DISABLED or ST_PENDING)
}_base_entry_t;

// object request flags
#define RQ_NAME		(1<<0) /*!< Class name */
#define RQ_INTERFACE	(1<<1) /*!< Interface name */
#define RQ_VERSION	(1<<2) /*!< Class version */
// deprecated
#define RQ_CMP_OR	(1<<7) /*!< comparsion type OR (AND is by default) */

/**
 * Helper structure.
 *
 * Used to search for objects in arrays of _base_entry_t structures.
 */
typedef struct {
	_u8 		flags;     /*!< @brief Flags for searching.\n
				    *	This field can be combination of:\n
				    *	RQ_NAME - search for classname (cname)\n
				    *	RQ_INTERFACE - search for interface name (iname)\n
				    *	RQ_VERSION - search for spec. version
				    */
	_cstr_t		cname;     //!< Const pointer to class name (const char *)
	_cstr_t		iname;     //!< Const pointer to  interface name (const char *)
	_version_t	version;
}_object_request_t;

/**
 * Object handle (pointer to _base_entry_t structure) */
typedef _base_entry_t* HOBJECT;

/**
 * @brief Prototype of enumeration callback.
 * @param[in] file - File name of a extension (shared library)
 * @param[in] alias - Extension alias name.
 * @param[in] p_bentry - Pointer to array of _base_entry_t structures.
 * @param[in] count - Number of _base_entry_t structures in array.
 * @param[in] limit - Limit of the array.
 * @param[in] udata - Pointer to user data.  */
typedef void _cb_enum_ext_t(_cstr_t file, _cstr_t alias, _base_entry_t *p_bentry, _u32 count, _u32 limit, void *udata);

/**
 * Interface of a 'repository' component.
 *
 * The repository is a so called spine of a postlink conception.
 * It's used to manage a components and load/unload/enumeration of extensions.  */
class iRepository: public iBase {
public:
	INTERFACE(iRepository, I_REPOSITORY);
/**
 * Retrieve iBase pointer by _object_request_t structure.
 *
 * This function can return NULL, if object not found or flags does not matched.
 * @param[in] request - Pointer to _object_request_t structure, that contains request information.
 * @param[in] flags - Repository flags (RF_ORIGINAL, RF_CLONE or both).
 * @return iBase* or null.  */
	virtual iBase *object_request(_object_request_t *, _rf_t)=0;
/**
 * Release iBase pointer.
 * @param[in] ptr - Pointer to iBase
 * @return iBase* or NULL.  */
	virtual void   object_release(iBase *)=0;
/**
 * Retrieve iBase pointer by class name.
 *
 * This function can return NULL, if component not found or flags does not matched.
 * @param[in] cname - const pointer to class name (const char *)
 * @param[in] flags - Repository flags (RF_ORIGINAL, RF_CLONE or both).
 * @return iBase* or NULL.  */
	virtual iBase *object_by_cname(_cstr_t cname, _rf_t)=0;
/**
 * Retrieve iBase pointer by interface name.
 *
 * This function can return NULL, if component not found or flags does not matched.
 * @param[in] iname - const pointer to interface name (const char *)
 * @param[in] flags - Repository flags (RF_ORIGINAL, RF_CLONE or both).
 * @return iBase* or NULL.  */
	virtual iBase *object_by_iname(_cstr_t iname, _rf_t)=0;
/**
 * Retrieve iBase pointer by handle (HOBJECT).
 *
 * This function can return NULL, if flags does not matched.
 * @param[in] handle - HOBJECT retrieved by handle_by_cname or handle_by_iname
 * @param[in] flags - Repository flags (RF_ORIGINAL, RF_CLONE or both).
 * @return iBase* or NULL.  */
	virtual iBase *object_by_handle(HOBJECT, _rf_t)=0;
/**
 * Return HOBJECT by interface name.
 *
 * This function can return NULL, if component not found.
 * @param[in] iname Const pointer to interface name (const char *)
 * @return HOBJECT or NULL  */
	virtual HOBJECT handle_by_iname(_cstr_t iname)=0;
/**
 * Return HOBJECT by class name.
 *
 * This function can return NULL, if component not found.
 * @param[in] cname Const pointer to class name (const char *)
 * @return HOBJECT or NULL  */
	virtual HOBJECT handle_by_cname(_cstr_t cname)=0;
/**
 * Retrieve object information by object handle.
 * @param[in] h - handle
 * @param[out] poi - Pointer to _object_info_t structure.
 * @return true/false  */
	virtual bool object_info(HOBJECT h, _object_info_t *poi)=0;

/**
 * Initialize array of _base_entry_t structures.
 *
 * This function usually used by the repository,
 * to initialize a newly loaded extensions.
 * The reason to present in public section is because
 * the startup code, uses this function for initialization of
 * core components array.
 *
 * @param[in] array - Array of _base_entry_t structures.
 * @param[in] count - Number of elements in array  */
	virtual void init_array(_base_entry_t *array, _u32 count)=0;

	// extensions

/**
 * Set path to extensions.
 *
 * @param[in] dir - Full or relative path to directory
 *                  that contains ext*.so files.  */
	virtual void extension_dir(_cstr_t dir)=0;
/**
 * Get path to extensions.
 *
 * @return Full or relative path as const pointer to null terminated string.  */
	virtual _cstr_t extension_dir(void)=0;
/**
 * Load extension.
 *
 * @param[in] file - Extension file name.
 * @param[in] alias - Alias name for the extension.
 *			If this parameter is not set (null),
 *			alias will be equal to file name
 * @return ERR_NONE for sucess, otherwise, ERR_MISSING for missing file or
 * ERR_DUPLICATED for already loaded extension or ERR_LOADEXT for other mistakes. */
	virtual _err_t extension_load(_cstr_t file, _cstr_t alias=0)=0;
/**
 * Unload extension
 *
 * @param[in] alias - Aloas name of loaded extension
 * @return ERR_NONE for success or ERR_MISSING when extension can not be found */
	virtual _err_t extension_unload(_cstr_t alias)=0;
/**
 * Enumeration of loaded extensions.
 *
 * @param[in] pcb - Pointer to enumeration callback.\n
 * void _cb_enum_ext_t(_cstr_t file, _cstr_t alias, _base_entry_t *p_bentry, _u32 count, _u32 limit, void *udata)
 * @param[in] udata - User data that will be passed to callback. */
	virtual void extension_enum(_cb_enum_ext_t *pcb, void *udata)=0;
	virtual void destroy(void)=0;
};

extern iRepository *_gpi_repo_;

#define BY_INAME(iname, flags) \
	_gpi_repo_->object_by_iname(iname, flags)
#define BY_CNAME(cname, flags) \
	_gpi_repo_->object_by_cname(cname, flags)

#endif


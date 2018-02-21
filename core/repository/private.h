#ifndef __REPO_PRIVATE_H__
#define __REPO_PRIVATE_H__

#include "startup.h"
#include "err.h"

#define I_REPO_EXTENSION	"iRepoExtension"

class iRepoExtension: public iBase {
public:
	INTERFACE(iRepoExtension, I_REPO_EXTENSION);
	virtual _str_t alias(void)=0;
	virtual _err_t load(_cstr_t file, _cstr_t alias=0)=0;
	virtual _err_t init(iRepository *)=0;
	virtual _err_t unload(void)=0;
	virtual _str_t file(void)=0;
	virtual void *address(void)=0;
	virtual _base_entry_t *array(_u32 *count, _u32 *limit)=0; // return pointer to object vector
	//...
};

#endif

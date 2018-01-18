#ifndef __REPO_PRIVATE_H__
#define __REPO_PRIVATE_H__

#include "startup.h"
#include "err.h"

#define I_REPO_EXTENSION	"iRepoExtension"

class iRepoExtension: public iBase {
public:
	INTERFACE(iRepoExtension, I_REPO_EXTENSION);
	virtual _str_t alias(void)=0;
	virtual _err_t load(_str_t file, _str_t alias=0)=0;
	virtual _err_t init(iRepository *)=0;
	virtual _err_t unload(void)=0;
	virtual _str_t file(void)=0;
	virtual void *address(void)=0;
	virtual _base_vector_t *vector(void)=0; // return pointer to object vector
	//...
};

#endif

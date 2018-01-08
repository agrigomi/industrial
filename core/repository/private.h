#ifndef __REPO_PRIVATE_H__
#define __REPO_PRIVATE_H__

#include "iBase.h"

#define I_REPO_EXTENSION	"iRepoExtension"

class iRepoExtension: public iBase {
public:
	INTERFACE(iRepoExtension, I_EXTENSION);
	//...
};

#endif

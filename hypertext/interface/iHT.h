#ifndef __I_HT_H__
#define __I_HT_H__

#include "iBase.h"

#define I_HT	"iHT"

#define HTCONTEXT	void*

class iHT:public iBase {
public:
	INTERFACE(iHT, I_HT);
	virtual HTCONTEXT create_context(void)=0;
	virtual void destroy_context(HTCONTEXT)=0;
};

#endif

#ifndef __EXT_1_PRIVATE_H__
#define __EXT_1_PRIVATE_H__

#include "iBase.h"

#define I_OBJ1	"iObj1"

class iObj_1: public iBase {
public:
	INTERFACE(iObj_1, I_OBJ1);

	virtual void method_1(void)=0;
	virtual void method_2(void)=0;
};

#endif

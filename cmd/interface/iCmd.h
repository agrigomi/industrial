#ifndef __I_CMD_H__
#define __I_CMD_H__

#include "iIO.h"

#define I_CMD	"iCmd"

class iCmd: public iBase {
public:
	INTERFACE(iCmd, I_CMD);
};

#endif


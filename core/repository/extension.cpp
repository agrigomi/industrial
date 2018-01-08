#include "iRepository.h"

class cExtension: public iExtension {
private:
public:
	BASE(cExtension, "cExtension", RF_CLONE, 1, 0,0);

	bool object_ctl(_u32 cmd, void *arg) {
		switch(cmd) {
			case OCTL_INIT: {
				//...
				r = true;
				break;
			}
			case OCTL_UNINIT: {
				//...
				r = true;
				break;
			}
		}
	}

	//...
};

static cExtension _g_object_;

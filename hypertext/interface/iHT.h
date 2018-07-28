#ifndef __I_HT_H__
#define __I_HT_H__

#include "iBase.h"

#define I_HT	"iHT"
#define I_XML	"iXML"

#define HTCONTEXT	void*
#define HTTAG		void*

class iHT:public iBase {
public:
	INTERFACE(iHT, I_HT);
	virtual HTCONTEXT create_context(void)=0;
	virtual void destroy_context(HTCONTEXT)=0;
	virtual bool parse(HTCONTEXT, _str_t, _ulong)=0;
	virtual HTTAG select(HTCONTEXT, _cstr_t xpath, HTTAG, _u32)=0;
	virtual _str_t parameter(HTCONTEXT, HTTAG, _cstr_t, _u32 *)=0;
	virtual _str_t content(HTTAG, _u32 *)=0;
};

class iXML: public iHT {
public:
	INTERFACE(iXML, I_XML);
};

#endif

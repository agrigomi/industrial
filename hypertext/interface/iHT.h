#ifndef __I_HT_H__
#define __I_HT_H__

#include "iBase.h"

#define I_XML	"iXML"

#define HTCONTEXT	void*
#define HTTAG		void*

class iXML:public iBase {
public:
	INTERFACE(iXML, I_XML);
	// Create parser context
	virtual HTCONTEXT create_context(void)=0;
	// destroy parser context
	virtual void destroy_context(HTCONTEXT)=0;
	// Parse (indexing) hypertext content
	virtual bool parse(HTCONTEXT, // parser context
			_cstr_t, // Content of hypertext
			_ulong // Size of hypertext content
			)=0;
	// Select hypertext tag
	virtual HTTAG select(HTCONTEXT, // parser context
			_cstr_t xpath, // path to tag (or NULL for inddex only enumeration)
			HTTAG, // start point (or NULL for root tag)
			_u32 // index
			)=0;
	// Get parameter value
	virtual _str_t parameter(HTCONTEXT, // parser context
			HTTAG, // tag handle (by select)
			_cstr_t, // parameter name
			_u32 * // [Out] Value size in symbols (not bytes)
			)=0;
	// get tag name
	virtual _str_t name(HTTAG, // tag handle
			_u32 *sz // [Out] size of tag name
			)=0;
	// Get tag content
	virtual _str_t content(HTTAG, // tag handle (by select)
			_u32 * // [Out] content size in symbols (not bytes)
			)=0;
	// Compare strins
	virtual _s32 compare(HTCONTEXT,
			_str_t, // string 1
			_str_t, // string 2
			_u32 // size
			)=0;
};

#endif

#ifndef __I_HT_H__
#define __I_HT_H__

#include "iBase.h"

#define I_XML	"iXML"
#define I_JSON	"iJSON"

#define HTCONTEXT	void*

class iHT: public iBase {
public:
	INTERFACE(iHT, "iHT");
	// Create parser context
	virtual HTCONTEXT create_context(void)=0;
	// destroy parser context
	virtual void destroy_context(HTCONTEXT)=0;
	// Parse (indexing) hypertext content
	virtual bool parse(HTCONTEXT, // parser context
			_cstr_t, // Content of hypertext
			_ulong // Size of hypertext content
			)=0;
	// Compare strins (according content encoding)
	virtual _s32 compare(HTCONTEXT,
			_str_t, // string 1
			_str_t, // string 2
			_u32 // size
			)=0;
};

#define HTTAG		void*

class iXML:public iHT {
public:
	INTERFACE(iXML, I_XML);
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
};

/* JSON value types */
#define JVT_STRING	1
#define JVT_NUMBER	2
#define JVT_OBJECT	3
#define JVT_ARRAY	4

typedef struct i_json_value _i_json_value_t;

struct i_json_value {
	virtual _u8 type(void)=0; // returns JASON value type
	virtual _i_json_value_t *by_index(_u32 index)=0; // for arrays and objects
	virtual _cstr_t data(_u32 *size)=0; // for strings and numbers
};

class iJSON: public iHT {
public:
	INTERFACE(iJSON, I_JSON);
	// Select value
	virtual _i_json_value_t *select(HTCONTEXT, // parser context
					_cstr_t jpath, // path to value
					_i_json_value_t * // start point JVT_OBJECT only (can be NULL)
					)=0;
	virtual _i_json_value_t *root(HTCONTEXT)=0; // returns ROOT object (JVT_OBJECT)
};

#endif

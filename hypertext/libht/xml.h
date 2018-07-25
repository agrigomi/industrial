#ifndef __XML_H__
#define __XML_H__

#include "context.h"

#define XML_OK			0
#define XML_PARSE_ERROR		-1
#define XML_MEMORY_ERROR	-2

/* tag flags */
#define	TF_ONELINE	(1<<0)
#define TF_EMBEDDED	(1<<1)
#define TF_SKIP		(1<<2)
#define TF_HOOK_PARSE	(1<<8)
#define TF_HOOK_EXPORT	(1<<9)

typedef int _xml_err_t;
typedef struct ht_tag _ht_tag_t;

typedef _xml_err_t _tag_hook_t(_ht_context_t *p_htc, /* hypertext context */
				unsigned int flags, /* one bit (TF_HOOK_PARSE | TF_HOOK_EXPORT) */
				_ht_tag_t *p_tag, /* tag info */
				void *p_udata /* user data */
				);

typedef struct {
	const unsigned char	*p_tag_name; /* tag name */
	unsigned int		flags; /* flags */
	const unsigned char	*p_parent_name; /* parent tag name (can be NULL) */
	_tag_hook_t		*pf_hook; /* callback proc. (can be NULL) */
	void			*p_udata; /* user data (can be NULL) */
} _tag_def_t;


struct ht_tag {
	unsigned char	*p_name;	/* tag name */
	unsigned char	sz_name;	/* sizeof tag name */
	unsigned char	*p_content;	/* tag content */
	unsigned int	sz_content;	/* size of tag content */
	unsigned char	*p_parameters;	/* tag parameters */
	unsigned short	sz_parameters;	/* sizeof tag parameters */
	_ht_tag_t	**pp_childs;	/* list of child tags */
	unsigned int	num_childs;	/* number of child tags */
	_tag_def_t	*p_tdef;	/* tag definition (can ne NULL) */
};

typedef struct {
	_ht_context_t	*p_htc;		/* hypertext context */
	_ht_tag_t	*p_root;	/* list of root tags */
	_tag_def_t	**pp_tdef;	/* array of tag_definitions */
	unsigned long	err_pos;	/* error position */
	unsigned int	num_tdef;	/* number of tag definitions */
} _xml_context_t;

#ifdef __cplusplus
extern "C" {
#endif
/* allocate memory for XML context */
_xml_context_t *xml_create_context(_mem_alloc_t *, _mem_free_t *);
/* parse XML content */
_xml_err_t xml_parse(_xml_context_t *p_xc, /* XML context */
			unsigned char *p_xml_content, /* XML content */
			unsigned long sz_xml_content /* size of XML content */
			);
_ht_tag_t *xml_select(_xml_context_t *p_xc,
			const char *xpath,
			_ht_tag_t *p_start_point, /* start tag (can be NULL) */
			unsigned int index);
/* get parameter value */
char *xml_tag_parameter(_xml_context_t *,_ht_tag_t *p_tag,
			const char *pname, unsigned int *sz);
void xml_add_tdef_array(_xml_context_t *p_xc, /* XML context */
			_tag_def_t *p_tdef_array /* array of tag definitions, terminated by empty struct. */
			);
void xml_remove_tdef_array(_xml_context_t *p_xc, /* XML context */
			_tag_def_t *p_tdef_array /* array of tag definitions, terminated by empty struct. */
			);
/* destroy (deallocate) XML context */
void xml_destroy_context(_xml_context_t *p_xc);
#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif

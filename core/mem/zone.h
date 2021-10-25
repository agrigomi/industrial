#ifndef __ZONE_H__
#define __ZONE_H__

#define ZONE_PAGE_SIZE		4096
#define ZONE_MIN_ALLOC		16
#define ZONE_MAX_ENTRIES	63
#define ZONE_BITMAP_SIZE	(ZONE_PAGE_SIZE / ZONE_MIN_ALLOC / 8) /* in bytes */
#define ZONE_BITMAP_UNIT	sizeof(long long)
#define ZONE_BITMAP_UNIT_BITS	64
#define ZONE_MAX_ZONES		(ZONE_PAGE_SIZE / sizeof(_zone_page_t *))
#define ZONE_DEFAULT_LIMIT	0xffffffffffffffffLLU

typedef struct zone_header	_zone_header_t;
typedef struct zone_page	_zone_page_t;
typedef struct zone_entry	_zone_entry_t;

struct zone_header {
	unsigned long long next; /* pointer to next zone page */
	unsigned int objects; /* number of objects in zone page */
	unsigned long object_size; /* (max. or first) object size in bytes */
	unsigned char reserved[44]; /* unused */
}__attribute__((packed));

struct zone_entry {
	union {
		unsigned long long bitmap[ZONE_BITMAP_SIZE / ZONE_BITMAP_UNIT]; /* used in cases: data size < page size */
		unsigned long data_size; /* used in cases: data size >= page size */
	};
	unsigned long long data; /* pointer to data region */
	unsigned int objects; /* number of reserved objects */
	unsigned char reserved[20]; /* unused */
}__attribute__((packed));

struct zone_page {
	_zone_header_t	header; /* zone header */
	_zone_entry_t	array[ZONE_MAX_ENTRIES]; /* zone entries */
}__attribute__((packed));

/* prototype callback: page allocator */
typedef void *_page_alloc_t(unsigned int num_pages, unsigned long long limit, void *user_data);
/* prototype callback: page deallocator */
typedef void _page_free_t(void *page_ptr, unsigned int num_pages, void *user_data);
/* prototype callback: mutex lock */
typedef unsigned long long _mutex_lock_t(unsigned long long mutex_handle, void *user_data);
/* prototype callback: mutex unlock */
typedef void _mutex_unlock_t(unsigned long long mutex_handle, void *user_data);

typedef struct {
	_page_alloc_t	*pf_page_alloc;
	_page_free_t	*pf_page_free;
	_mutex_lock_t	*pf_mutex_lock;
	_mutex_unlock_t	*pf_mutex_unlock;

	_zone_page_t	**zones; /* initially must be NULL */
	unsigned long long limit;
	void		*user_data;
}_zone_context_t;

#ifdef __cplusplus
extern "C" {
#endif
/* Initialize zone context and return 0 for success */
int zone_init(_zone_context_t *p_zcxt);
/* Allocates chunk of memory and returns pointer or NULL */
void *zone_alloc(_zone_context_t *p_zcxt, unsigned int size, unsigned long long limit);
/* Deallocate memory chunk and returns 0 for success */
int zone_free(_zone_context_t *p_zcxt, void *ptr, unsigned int size);
/* Verify pointer. Returns 1 if pointer and size, belongs to active object */
int zone_verify(_zone_context_t *p_zcxt, void *ptr, unsigned int size);
/* Destroy zone context */
void zone_destroy(_zone_context_t *p_zcxt);
#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif

#include "zone.h"

/* aligns size to next degree on 2 */
static unsigned int _align(unsigned int size) {
	unsigned int r = 0;

	if(size && size < ZONE_MIN_ALLOC)
		r = ZONE_MIN_ALLOC;
	else if(size > ZONE_PAGE_SIZE) {
		r = size / ZONE_PAGE_SIZE;
		r += (size % ZONE_PAGE_SIZE) ? 1 : 0;
		r *= ZONE_PAGE_SIZE;
	} else if(size && size < ZONE_PAGE_SIZE) {
		unsigned int m = 0;

		r = ZONE_PAGE_SIZE;
		while(r && !(r & size))
			r >>= 1;

		m = r >> 1;
		while(m && !(m & size))
			m >>= 1;

		if(m)
			r <<= 1;
	} else
		r = size;

	return r;
}

static void _clear(void *ptr, unsigned int size) {
	unsigned int n = size / sizeof(unsigned long long);
	unsigned long long *p = (unsigned long long *)ptr;
	unsigned int i = 0;

	for(; i < n; i++)
		*(p + i) = 0;
}

static unsigned long long _lock(_zone_context_t *p_zcxt, unsigned long long mutex_handle) {
	unsigned long long r = 0;

	if(p_zcxt->pf_mutex_lock)
		r = p_zcxt->pf_mutex_lock(mutex_handle, p_zcxt->user_data);

	return r;
}

static void _unlock(_zone_context_t *p_zcxt, unsigned long long mutex_handle) {
	if(p_zcxt->pf_mutex_unlock)
		p_zcxt->pf_mutex_unlock(mutex_handle, p_zcxt->user_data);
}

static unsigned int _bit_number(unsigned int x) {
	unsigned int mask_h = ((unsigned int)1 << 31);
	unsigned int h2l_pos = 0;

	while(!(mask_h & x)) {
		mask_h >>= 1;
		h2l_pos++;
	}

	return (sizeof(unsigned int) << 3) - h2l_pos;
}

static _zone_page_t **_zone_page(_zone_context_t *p_zcxt, unsigned int size, unsigned int *aligned_size) {
	_zone_page_t **r = (_zone_page_t **)0;
	unsigned int zone_index = _bit_number((*aligned_size = _align(size)));

	if(zone_index && zone_index < ZONE_MAX_ZONES) /* because index 0 not allowed */
		r = p_zcxt->zones + zone_index;

	return r;
}

/* Initialize zone context and return 0 for success */
int zone_init(_zone_context_t *p_zcxt) {
	int r = -1;

	if(p_zcxt->pf_page_alloc && p_zcxt->pf_page_free && !p_zcxt->zones)
		p_zcxt->zones = (_zone_page_t **)p_zcxt->pf_page_alloc(1, p_zcxt->limit, p_zcxt->user_data);

	if(p_zcxt->zones) {
		_clear(p_zcxt->zones, ZONE_PAGE_SIZE);
		r = 0;
	}

	return r;
}

/* returns unit as result and bit number in 'bit' parameter
	'bit' parameter must contains a requested bit
*/
static unsigned long long *_bitmap_unit_bit(_zone_entry_t *p_entry,
					unsigned int aligned_size,
					unsigned char *bit // [in / out]
					) {
	unsigned long long *r = (unsigned long long *)0;
	unsigned int max_bits = ZONE_PAGE_SIZE / aligned_size;

	if(*bit < max_bits) {
		unsigned char unit = *bit / ZONE_BITMAP_UNIT_BITS;
		*bit = *bit % ZONE_BITMAP_UNIT_BITS;
		r = &p_entry->bitmap[unit];
	}

	return r;
}

static void *_zone_entry_alloc(_zone_context_t *p_zcxt, _zone_entry_t *p_entry, unsigned int aligned_size, unsigned long long limit) {
	void *r = (void *)0;

	if(aligned_size < ZONE_PAGE_SIZE) {
		unsigned int unit = 0;
		unsigned int bit = 0, unit_bit = 0;
		unsigned int max_bits = ZONE_PAGE_SIZE / aligned_size;
		unsigned long long mask = ((unsigned long long)1 << (ZONE_BITMAP_UNIT_BITS -1));

		while(bit < max_bits) {
			if(unit_bit >= ZONE_BITMAP_UNIT_BITS) {
				unit_bit = 0;
				unit++;
			}

			if(!(p_entry->bitmap[unit] & (mask >> unit_bit)))
				break;

			bit++;
			unit_bit++;
		}

		if(bit < max_bits) {
			unsigned char *data = (unsigned char *)p_entry->data;

			if(!data) {
				data = (unsigned char *)p_zcxt->pf_page_alloc(1, limit, p_zcxt->user_data);
				p_entry->data = (unsigned long long)data;
			}

			if(data) {
				r = data + (bit * aligned_size);
				p_entry->bitmap[unit] |= (mask >> unit_bit);
				p_entry->objects++;
			}
		}
	} else {
		/* one object per entry */
		if(p_entry->objects == 0) {
			unsigned char *data = (unsigned char *)p_entry->data;

			if(!data) {
				data = (unsigned char *)p_zcxt->pf_page_alloc(aligned_size / ZONE_PAGE_SIZE, limit, p_zcxt->user_data);
				p_entry->data = (unsigned long long)data;
			}

			if(data) {
				r = data;
				p_entry->objects = 1;
				p_entry->data_size = aligned_size;
			}
		}
	}

	return r;
}

static void *_zone_page_alloc(_zone_context_t *p_zcxt, _zone_page_t *p_zone, unsigned int aligned_size, unsigned long long limit) {
	void *r = (void *)0;
	unsigned int max_objects = ZONE_MAX_ENTRIES;
	_zone_page_t *p_current_page = p_zone;
	unsigned long long mutex_handle = 0;

	if(aligned_size < ZONE_PAGE_SIZE)
		max_objects = (ZONE_MAX_ENTRIES * ZONE_PAGE_SIZE) / aligned_size;

	mutex_handle = _lock(p_zcxt, 0);
	while(p_current_page && p_current_page->header.objects == max_objects) {
		if(!(p_current_page->header.next)) {
			/* alloc new page header */
			_zone_page_t *p_new_page = (_zone_page_t *)p_zcxt->pf_page_alloc(1, limit, p_zcxt->user_data);

			if(p_new_page) {
				_clear(p_new_page, sizeof(_zone_page_t));
				p_new_page->header.object_size = aligned_size;
			}

			p_current_page->header.next = (unsigned long long)p_new_page;
		}

		/* swith to next page */
		p_current_page = (_zone_page_t *)p_current_page->header.next;
	}

	if(p_current_page) {
		unsigned int i = 0;
		unsigned int max_entry_objects = 1;

		if(aligned_size < ZONE_PAGE_SIZE)
			max_entry_objects = ZONE_PAGE_SIZE / aligned_size;

		while(i < ZONE_MAX_ENTRIES) {
			if(p_current_page->array[i].objects < max_entry_objects) {
				if((r = _zone_entry_alloc(p_zcxt, &p_current_page->array[i], aligned_size, limit)))
					p_current_page->header.objects++;
				break;
			}

			i++;
		}
	}
	_unlock(p_zcxt, mutex_handle);

	return r;
}

/* Allocates chunk of memory and returns pointer or NULL */
void *zone_alloc(_zone_context_t *p_zcxt, unsigned int size, unsigned long long limit) {
	void *r = (void *)0;
	unsigned int aligned_size = 0;
	_zone_page_t **pp_zone = _zone_page(p_zcxt, size, &aligned_size);

	if(pp_zone) {
		_zone_page_t *p_zone = *pp_zone;

		if(!p_zone) { // alloc zone page
			if((p_zone = (_zone_page_t *)p_zcxt->pf_page_alloc(1, limit, p_zcxt->user_data))) {
				unsigned long long mutex_handle = 0;

				_clear(p_zone, sizeof(_zone_page_t));

				mutex_handle = _lock(p_zcxt, 0);
				*pp_zone = p_zone;
				p_zone->header.object_size = aligned_size;
				_unlock(p_zcxt, mutex_handle);
			}
		}

		if(p_zone)
			r = _zone_page_alloc(p_zcxt, p_zone, aligned_size, limit);
	}

	return r;
}

/* Deallocate memory chunk */
int zone_free(_zone_context_t *p_zcxt, void *ptr, unsigned int size) {
	int r = 1;
	unsigned int aligned_size = 0;
	_zone_page_t **pp_zone = _zone_page(p_zcxt, size, &aligned_size);
	unsigned long long mutex_handle = 0;

	if(pp_zone) {
		_zone_page_t *p_zone = *pp_zone;

		mutex_handle = _lock(p_zcxt, mutex_handle);
		while(p_zone) {
			unsigned int i = 0;

			while(i < ZONE_MAX_ENTRIES) {
				_zone_entry_t *p_entry = &p_zone->array[i];
				void *data = (void *)p_entry->data;

				if(ptr >= data && ptr < (data + ZONE_PAGE_SIZE)) {
					if(aligned_size < ZONE_PAGE_SIZE) {
						unsigned char bit = (ptr - data) / aligned_size;
						unsigned long long *unit = _bitmap_unit_bit(p_entry, aligned_size, &bit);
						unsigned long long mask = ((unsigned long long)1 << (ZONE_BITMAP_UNIT_BITS -1));

						if(unit && (*unit & (mask >> bit))) {
							*unit &= ~(mask >> bit);
							p_entry->objects--;
							p_zone->header.objects--;
							r = 0;
						}
						goto _zone_free_end_;
					} else {
						if(p_entry->objects && p_entry->data_size == aligned_size) {
							p_zcxt->pf_page_free(data, aligned_size / ZONE_PAGE_SIZE, p_zcxt->user_data);
							p_entry->data_size = 0;
							p_entry->data = 0;
							p_entry->objects = 0;
							p_zone->header.objects--;
							r = 0;
						}
						goto _zone_free_end_;
					}
				}

				i++;
			}

			p_zone = (_zone_page_t *)p_zone->header.next;
		}
_zone_free_end_:
		_unlock(p_zcxt, mutex_handle);
	}

	return r;
}

/* Verify pointer. Returns 1 if pointer belongs to active object */
int zone_verify(_zone_context_t *p_zcxt, void *ptr, unsigned int size) {
	int r = 0;
	unsigned int aligned_size = 0;
	_zone_page_t **pp_zone = _zone_page(p_zcxt, size, &aligned_size);
	unsigned long long mutex_handle = 0;

	if(pp_zone) {
		_zone_page_t *p_zone = *pp_zone;

		mutex_handle = _lock(p_zcxt, mutex_handle);
		while(p_zone) {
			unsigned int i = 0;

			while(i < ZONE_MAX_ENTRIES) {
				_zone_entry_t *p_entry = &p_zone->array[i];
				void *data = (void *)p_entry->data;

				if(ptr >= data && ptr < (data + ZONE_PAGE_SIZE)) {
					if(aligned_size < ZONE_PAGE_SIZE) {
						unsigned char bit = (ptr - data) / aligned_size;
						unsigned long long *unit = _bitmap_unit_bit(p_entry, aligned_size, &bit);
						unsigned long long mask = ((unsigned long long)1 << (ZONE_BITMAP_UNIT_BITS -1));

						if(unit && (*unit & (mask >> bit)))
							r = 1;
						goto _zone_verify_end_;
					} else {
						if(p_entry->objects && p_entry->data_size == aligned_size)
							r = 1;
						goto _zone_verify_end_;
					}
				}

				i++;
			}

			p_zone = (_zone_page_t *)p_zone->header.next;
		}
_zone_verify_end_:
		_unlock(p_zcxt, mutex_handle);
	}

	return r;
}

static void destroy_zone_page(_zone_context_t *p_zcxt, _zone_page_t *p_zone) {
	unsigned int idx = 0;
	_zone_page_t *p_current_zone = p_zone;

	while(p_current_zone) {
		_zone_page_t *p_next_zone = (_zone_page_t *)p_current_zone->header.next;

		while(idx < ZONE_MAX_ENTRIES) {
			_zone_entry_t *p_entry = &p_current_zone->array[idx];

			if(p_entry->data) {
				unsigned int data_pages = 1;

				if(p_zone->header.object_size > ZONE_PAGE_SIZE)
					data_pages = p_entry->data_size / ZONE_PAGE_SIZE;

				p_zcxt->pf_page_free((void *)p_entry->data, data_pages, p_zcxt->user_data);
			}

			idx++;
		}

		p_zcxt->pf_page_free(p_current_zone, 1, p_zcxt->user_data);
		p_current_zone = p_next_zone;
	}
}

/* Destroy zone context */
void zone_destroy(_zone_context_t *p_zcxt) {
	_zone_page_t *p_zone = (_zone_page_t *)0;
	unsigned int idx = 0;
	unsigned int max_zones = ZONE_PAGE_SIZE / sizeof(_zone_page_t *);
	unsigned long long mutex_handle = _lock(p_zcxt, 0);

	if(p_zcxt->zones) {
		while(idx < max_zones) {
			if((p_zone = p_zcxt->zones[idx]))
				destroy_zone_page(p_zcxt, p_zone);

			idx++;
		}

		p_zcxt->pf_page_free(p_zcxt->zones, 1, p_zcxt->user_data);
	}

	_unlock(p_zcxt, mutex_handle);
}

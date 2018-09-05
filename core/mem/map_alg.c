#include <string.h>
#include "map_alg.h"

/* make long integer from string */
static _ulong hash(_str_t str, _u32 sz) {
	_ulong hash = 5381;
	_u32 c, n=0;

	while(n < sz) {
		c = str[n];
		hash = ((hash << 5) + hash) + c; /* hash * 33 + c */
		n++;
	}

	return hash;
}

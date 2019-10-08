#include "private.h"

void lm_clean(iBase *pi_base) {
	_u32 count;
	const _link_info_t *pl = pi_base->object_link(&count);

	if(pl) {
		for(_u32 i = 0; i < count; i++)
			*pl[i].ppi_base = NULL;
	}
}

_u32 lm_init(iBase *pi_base, _cb_object_request_t *pcb, void *udata) {
	_u32 r = PLMR_READY;
	_u32 count;
	const _link_info_t *pl = pi_base->object_link(&count);

	if(pl) {
		for(_u32 i = 0; i < count; i++) {
			if(*pl[i].ppi_base == NULL) {
				if(pl[i].flags & RF_KEEP_PENDING)
					// permanently pending
					r |= PLMR_KEEP_PENDING;

				if(!(pl[i].flags & RF_POST_INIT)) {
					if((*pl[i].ppi_base = pcb(&pl[i], udata))) {
						if(pl[i].p_ref_ctl)
							pl[i].p_ref_ctl(RCTL_REF, pl[i].udata);
					} else {
						if(!(pl[i].flags & RF_NOCRITICAL))
							r = PLMR_FAILED;
						else // pending once
							r |= PLMR_KEEP_PENDING;
					}
				}
			}
		}
	}

	return r;
}

_u32 lm_post_init(iBase *pi_base, _base_entry_t *p_bentry, _cb_create_object_t *pcb, void *udata) {
	_u32 r = 0;
	_u32 count;
	const _link_info_t *pl = pi_base->object_link(&count);

	if(pl) {
		for(_u32 i = 0; i < count; i++) {
			//...
		}
	}

	return r;
}

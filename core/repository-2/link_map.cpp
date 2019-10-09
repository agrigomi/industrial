#include <string.h>
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
						if(!(pl[i].flags & RF_NOCRITICAL)) {
							r = PLMR_FAILED;
							break;
						} else // pending once
							r |= PLMR_KEEP_PENDING;
					}
				}
			}
		}
	}

	return r;
}

static bool lm_compare(const _link_info_t *pli, iBase *pi_base) {
	bool r = false;
	_object_info_t oi;

	pi_base->object_info(&oi);

	if(pli->cname)
		r = (strcmp(pli->cname, oi.cname) == 0);
	else if(pli->iname)
		r = (strcmp(pli->iname, oi.iname) == 0);

	return r;
}

_u32 lm_post_init(iBase *pi_base, _base_entry_t *p_bentry, _cb_create_object_t *pcb, void *udata) {
	_u32 r = PLMR_READY;
	_u32 count;
	const _link_info_t *pl = pi_base->object_link(&count);

	if(pl) {
		for(_u32 i = 0; i < count; i++) {
			bool create = false;

			if(pl[i].flags & RF_KEEP_PENDING) {
				r |= PLMR_KEEP_PENDING;
				create = true;
			}

			if(*pl[i].ppi_base == NULL)
				create = true;

			if(create) {
				if(lm_compare(&pl[i], p_bentry->pi_base)) {
					iBase *p = pcb(p_bentry, pl[i].flags, udata);

					if(p) {
						*pl[i].ppi_base = p;
						if(pl[i].p_ref_ctl)
							pl[i].p_ref_ctl(RCTL_REF, pl[i].udata);
					}
				}
			}
		}
	}

	return r;
}

_u32 lm_uninit(iBase *pi_base, _cb_release_object_t *pcb, void *udata) {
	_u32 r = PLMR_READY;
	_u32 count;
	const _link_info_t *pl = pi_base->object_link(&count);

	if(pl) {
		for(_u32 i = 0; i < count; i++) {
			if(*pl[i].ppi_base) {
				if(pl[i].p_ref_ctl)
					pl[i].p_ref_ctl(RCTL_UNREF, pl[i].udata);
				pcb(*pl[i].ppi_base, udata);
				*pl[i].ppi_base = NULL;
			}
		}
	}

	return r;
}

_u32 lm_remove(iBase *pi_base, _base_entry_t *p_bentry, _cb_release_object_t *pcb, void *udata) {
	_u32 r = PLMR_READY;
	_u32 count;
	const _link_info_t *pl = pi_base->object_link(&count);

	if(pl) {
		for(_u32 i = 0; i < count; i++) {
			if(*pl[i].ppi_base && lm_compare(&pl[i], p_bentry->pi_base)) {
				if(pl[i].flags & RF_KEEP_PENDING)
					r |= PLMR_KEEP_PENDING;

				if(!(pl[i].flags & RF_NOCRITICAL))
					r = PLMR_UNINIT;

				if(pl[i].p_ref_ctl)
					pl[i].p_ref_ctl(RCTL_UNREF, pl[i].udata);
				pcb(*pl[i].ppi_base, udata);
				*pl[i].ppi_base = NULL;
			}
		}
	}

	return r;
}


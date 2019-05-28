#include "private.h"

bool cHttpClientConnection::object_ctl(_u32 cmd, void *arg, ...) {
	bool r = false;

	switch(cmd) {
		case OCTL_INIT:
			mpi_sio = NULL;
			mpi_bmap = dynamic_cast <iBufferMap *>(_gpi_repo_->object_by_iname(I_BUFFER_MAP, RF_CLONE | RF_NONOTIFY));
			mpi_map = dynamic_cast <iMap *>(_gpi_repo_->object_by_iname(I_MAP, RF_CLONE | RF_NONOTIFY));

			if(mpi_bmap && mpi_map) {
				if(mpi_map->init(31))
					r = true;
			}
			break;
		case OCTL_UNINIT:
			_gpi_repo_->object_release(mpi_sio);
			_gpi_repo_->object_release(mpi_map);
			_gpi_repo_->object_release(mpi_bmap);
			r = true;
			break;
	}

	return r;
}


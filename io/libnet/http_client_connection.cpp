#include "private.h"

#define INITIAL_BUFFER_ARRAY	16

bool cHttpClientConnection::object_ctl(_u32 cmd, void *arg, ...) {
	bool r = false;

	switch(cmd) {
		case OCTL_INIT:
			mpi_sio = NULL;
			mpi_map = dynamic_cast <iMap *>(_gpi_repo_->object_by_iname(I_MAP, RF_CLONE | RF_NONOTIFY));
			mpi_heap = dynamic_cast<iHeap *>(_gpi_repo_->object_by_iname(I_HEAP, RF_ORIGINAL));
			if(mpi_map && mpi_heap) {
				if(mpi_map->init(31))
					r = true;
			}
			break;
		case OCTL_UNINIT: {
			_gpi_repo_->object_release(mpi_sio);
			_gpi_repo_->object_release(mpi_map);

			for(_u32 i = 0; i < m_sz_barray; i++) {
				if(mpp_buffer_array[i])
					mpi_heap->free(mpp_buffer_array[i], m_buffer_size);
			}

			mpi_heap->free(mpp_buffer_array, m_sz_barray * sizeof(void *));

			_gpi_repo_->object_release(mpi_heap);
			r = true;
		} break;
	}

	return r;
}

bool cHttpClientConnection::_init(iSocketIO *pi_sio, _u32 buffer_size) {
	bool r = true;

	mpi_sio = pi_sio;
	m_buffer_size = buffer_size;
	mpp_buffer_array = NULL;
	m_sz_barray = 0;
	m_req_method = 0;
	m_content_len = 0;
	return r;
}

void *cHttpClientConnection::alloc_buffer(void) {
	void *r = NULL;

	//...

	return r;
}

void cHttpClientConnection::reset(void) {
	m_content_len = 0;
	m_req_method = 0;
	mpi_map->clr();
}

bool cHttpClientConnection::alive(void) {
	bool r = false;

	if(mpi_sio)
		r = mpi_sio->alive();

	return r;
}

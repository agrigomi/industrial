#include "iCmd.h"
#include "iStr.h"
#include "iRepository.h"
#include "iMemory.h"
#include "startup.h"

#define MAX_ARGV	128

IMPLEMENT_BASE_ARRAY("libcmd", 10)

typedef struct {
	_cstr_t	cmd_name;
	iCmd	*pi_cmd;
}_cmd_rec_t;

class cCmdHost: public iCmdHost {
private:
	iLlist	*mpi_cmd_list;
	iStr	*mpi_str;
	iHeap	*mpi_heap;
	HNOTIFY	mh_notify;

	_cmd_rec_t *find_command(iBase *pi_cmd, _str_t cmd_name) {
		_cmd_rec_t *r = 0;
		iCmd *pi_cmd_obj = dynamic_cast<iCmd *>(pi_cmd);

		if(mpi_cmd_list) {
			_u32 sz = 0;
			HMUTEX hm = mpi_cmd_list->lock();
			_cmd_rec_t *rec = (_cmd_rec_t *)mpi_cmd_list->first(&sz, hm);

			while(rec) {
				if(pi_cmd) {
					if(pi_cmd_obj == rec->pi_cmd) {
						r = rec;
						break;
					}
				}
				if(cmd_name) {
					if(mpi_str->str_cmp(cmd_name, (_str_t)rec->cmd_name) == 0 && rec->pi_cmd) {
						r = rec;
						break;
					}
				}
				rec = (_cmd_rec_t *)mpi_cmd_list->next(&sz, hm);
			}

			mpi_cmd_list->unlock(hm);
		}

		return r;
	}

	void add_command(iBase *pi_cmd) {
		iCmd *pi_cmd_obj = dynamic_cast<iCmd *>(pi_cmd);

		if(pi_cmd_obj) {
			_cmd_t *p_cmd_info = pi_cmd_obj->get_info();
			_u32 n = 0;

			while(p_cmd_info[n].cmd_name) {
				_cmd_rec_t rec;

				rec.cmd_name = p_cmd_info[n].cmd_name;
				rec.pi_cmd = pi_cmd_obj;

				mpi_cmd_list->add(&rec, sizeof(_cmd_rec_t));
				n++;
			}
		}
	}

	void remove_command(iBase *pi_cmd) {
		iCmd *pi_cmd_obj = dynamic_cast<iCmd *>(pi_cmd);

		if(pi_cmd_obj) {
			HMUTEX hm = mpi_cmd_list->lock();
			_u32 sz = 0;
			_cmd_rec_t *p_rec = (_cmd_rec_t *)mpi_cmd_list->first(&sz, hm);

			while(p_rec) {
				if(p_rec->pi_cmd == pi_cmd_obj) {
					mpi_cmd_list->del(hm);
					p_rec = (_cmd_rec_t *)mpi_cmd_list->current(&sz, hm);
				} else
					p_rec = (_cmd_rec_t *)mpi_cmd_list->next(&sz, hm);
			}

			mpi_cmd_list->unlock(hm);
		}
	}

#define INV_POS  0xffffffff

	_u32 parse_argv(_str_t cmd_line, _u32 cmd_len, _str_t argv[]) {
		_u32 r = 0;
		bool strophe = false;
		bool quotes = false;
		_u32 arg_pos = INV_POS;
		_u32 i = 0;

		while(i < cmd_len + 1) {
			switch(cmd_line[i]) {
				case '\'':
					mpi_str->mem_cpy(cmd_line+i, cmd_line+i+1, cmd_len - i);
					strophe = !strophe;
					continue;
				case '"':
					mpi_str->mem_cpy(cmd_line+i, cmd_line+i+1, cmd_len - i);
					quotes = !quotes;
					continue;
				case '\\':
					mpi_str->mem_cpy(cmd_line+i, cmd_line+i+1, cmd_len - i);
					break;
				case 0:
				case ' ':
					if(arg_pos != INV_POS) {
						if(!quotes && !strophe) {
							argv[r] = cmd_line + arg_pos;
							arg_pos = INV_POS;
							cmd_line[i] = 0;
							r++;
						}
					}
					break;
				default:
					if(arg_pos == INV_POS)
						arg_pos = i;
					break;
			}

			i++;
		}

		return r;
	}
public:
	BASE(cCmdHost, "cCmdHost", RF_ORIGINAL, 1,0,0);

	bool object_ctl(_u32 cmd, void *arg, ...) {
		bool r = false;

		switch(cmd) {
			case OCTL_INIT: {
					iRepository *pi_repo = (iRepository *)arg;
					mpi_str = (iStr *)pi_repo->object_by_iname(I_STR, RF_ORIGINAL);
					mpi_heap = (iHeap*) pi_repo->object_by_iname(I_HEAP, RF_ORIGINAL);
					if((mpi_cmd_list = (iLlist *)pi_repo->object_by_iname(I_LLIST, RF_CLONE)))
						mpi_cmd_list->init(LL_VECTOR, 1);
					if(mpi_str && mpi_heap && mpi_cmd_list) {
						mh_notify = pi_repo->monitoring_add(0, I_CMD, 0, this, SCAN_ORIGINAL|SCAN_CLONE);
						r = true;
					}
				} break;
			case OCTL_UNINIT: {
					iRepository *pi_repo = (iRepository *)arg;
					if(mh_notify)
						pi_repo->monitoring_remove(mh_notify);
					if(mpi_str)
						pi_repo->object_release(mpi_str);
					if(mpi_heap)
						pi_repo->object_release(mpi_heap);
					if(mpi_cmd_list)
						pi_repo->object_release(mpi_cmd_list);
					r = true;
				} break;
			case OCTL_NOTIFY: {
					_notification_t *pn = (_notification_t *)arg;
					if(pn->flags & NF_INIT)
						add_command(pn->object);
					if(pn->flags & (NF_REMOVE|NF_UNINIT))
						remove_command(pn->object);
					r = true;
				} break;
		}

		return r;
	}

	_cmd_t *get_info(_str_t cmd_name, iCmd **pi_cmd=0) {
		_cmd_rec_t *cmd_rec = find_command(0, cmd_name);
		_cmd_t *r = (cmd_rec) ? cmd_rec->pi_cmd->get_info() : 0;

		if(r) {
			_u32 n = 0;

			while(r[n].cmd_name) {
				if(mpi_str->str_cmp(cmd_name, (_str_t)r[n].cmd_name) == 0) {
					r += n;
					if(pi_cmd)
						*pi_cmd = cmd_rec->pi_cmd;
					break;
				}
				n++;
			}

			if(r[n].cmd_name == 0)
				r = 0;
		}
		return r;
	}

	void exec(_str_t cmd_line, iIO *pi_io) {
		_str_t cmd=0;
		_u32 cmd_len = mpi_str->str_len(cmd_line) + 1;

		if((cmd = (_str_t)mpi_heap->alloc(cmd_len))) {
			_str_t argv[MAX_ARGV];
			_u32 argc = 0;
			_cmd_t *p_cmd = 0;
			iCmd *pi_cmd = 0;

			mpi_str->mem_set(argv, 0, sizeof(argv));
			mpi_str->str_cpy(cmd, cmd_line, cmd_len);
			argc = parse_argv(cmd, cmd_len, argv);
			if((p_cmd = get_info(argv[0], &pi_cmd))) {
				if(p_cmd->cmd_handler)
					p_cmd->cmd_handler(pi_cmd, this, pi_io, p_cmd->cmd_options, argc, argv);
			} else {
				if(pi_io) {
					_str_t ucmd = (_str_t)"Unknown command !\n";
					pi_io->write(ucmd, mpi_str->str_len(ucmd));
				}
			}

			mpi_heap->free(cmd, cmd_len);
		}
	}

	// check for option
	bool option_check(_str_t, _cmd_opt_t *) {
		bool r = false;
		//...
		return r;
	}
	// get option value
	_str_t option_value(_str_t, _cmd_opt_t *) {
		_str_t r = 0;
		//...
		return r;
	}
	// get argument (outside options list)
	_str_t arg_value(_u32 aidx, _cmd_opt_t *) {
		_str_t r = 0;
		//...
		return r;
	}
};

static cCmdHost _g_cmd_host_;

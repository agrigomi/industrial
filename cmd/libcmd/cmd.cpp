#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include "iCmd.h"
#include "iStr.h"
#include "iRepository.h"
#include "iMemory.h"
#include "startup.h"

#define MAX_ARGV		128
#define MAX_OPTION_NAME		128

IMPLEMENT_BASE_ARRAY("libcmd", 10)

typedef struct {
	_cstr_t	cmd_name;
	iCmd	*pi_cmd;
	volatile bool running;
}_cmd_rec_t;

class cCmdHost: public iCmdHost {
private:
	iMap	*mpi_cmd_map;
	iStr	*mpi_str;
	iHeap	*mpi_heap;

	_cmd_rec_t *find_command(_cstr_t cmd_name) {
		_cmd_rec_t *r = 0;
		_u32 sz = 0;

		if(mpi_cmd_map)
			r = (_cmd_rec_t *)mpi_cmd_map->get(cmd_name, strlen(cmd_name), &sz);

		return r;
	}

	// adds a new command to command linked list
	void add_command(iBase *pi_cmd) {
		iCmd *pi_cmd_obj = dynamic_cast<iCmd *>(pi_cmd);

		if(pi_cmd_obj) {
			_cmd_t *p_cmd_info = pi_cmd_obj->get_info();
			_u32 n = 0;

			while(p_cmd_info[n].cmd_name) {
				_cmd_rec_t rec;

				rec.cmd_name = p_cmd_info[n].cmd_name;
				rec.pi_cmd = pi_cmd_obj;
				rec.running = false;

				mpi_cmd_map->set(rec.cmd_name, strlen(rec.cmd_name), &rec, sizeof(_cmd_rec_t));
				n++;
			}
		}
	}

	// removes a command from linked list
	void remove_command(iBase *pi_cmd) {
		iCmd *pi_cmd_obj = dynamic_cast<iCmd *>(pi_cmd);

		if(pi_cmd_obj) {
			_cmd_t *p_cmd_info = pi_cmd_obj->get_info();
			mpi_cmd_map->del(p_cmd_info->cmd_name, strlen(p_cmd_info->cmd_name));
		}
	}

#define INV_POS  0xffffffff

	// converts a command line string to list of arguments
	_u32 parse_argv(_str_t cmd_line, _u32 cmd_len, _str_t argv[]) {
		_u32 r = 0;
		bool strophe = false;
		bool quotes = false;
		_u32 arg_pos = INV_POS;
		_u32 i = 0;

		while(i < cmd_len + 1 && r < MAX_ARGV) {
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
				case '\n':
				case '\r':
				case '\t':
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

	_u32 num_options(_cmd_opt_t *p_opt) {
		_u32 r = 0;

		if(p_opt) {
			while(p_opt[r].opt_name)
				r++;
		}

		return r;
	}

	_cmd_opt_t *find_option(_cstr_t opt_name, _cmd_opt_t *opt_array) {
		_cmd_opt_t *r = 0;
		_u32 n = 0;

		while(opt_array[n].opt_name) {
			if(mpi_str->str_cmp((_str_t)opt_name, (_str_t)opt_array[n].opt_name) == 0) {
				r = &opt_array[n];
				break;
			}
			n++;
		}

		return r;
	}

	void parse_options(_cmd_opt_t *p_opt_array, _u32 argc, _str_t argv[]) {
		_u32 i = 1;

		for(; i < argc; i++) {
			if(argv[i][0] == '-') {
				_cmd_opt_t *p_opt = 0;
				_char_t opt_name[MAX_OPTION_NAME]="";

				if(argv[i][1] == '-') {
					// long option
					_u32 j = 2, n = 0;

					while(argv[i][j] != 0 && argv[i][j] != '=' && argv[i][j] != ':') {
						opt_name[n] = argv[i][j];
						n++;
						j++;
					}

					opt_name[n] = 0;

					if((p_opt = find_option(opt_name, p_opt_array))) {
						if(p_opt->opt_flags & OF_LONG) {
							p_opt->opt_flags |= OF_PRESENT;
							if(p_opt->opt_flags & OF_VALUE) {
								if(argv[i][j] == '=' || argv[i][j] == ':')
									p_opt->opt_value = &argv[i][j+1];
								else {
									if(argv[i][j] == 0 && i < argc - 1) {
										p_opt->opt_value = argv[i+1];
										i++;
									}
								}
							}
						}
					}
				} else {
					// short options
					_u32 j = 1;
					while(argv[i][j]) {
						opt_name[0] = argv[i][j];
						opt_name[1] = 0;

						if((p_opt = find_option(opt_name, p_opt_array))) {
							p_opt->opt_flags |= OF_PRESENT;
							if(p_opt->opt_flags & OF_VALUE) {
								if(argv[i][j+1] != 0) {
									p_opt->opt_value = &argv[i][j+1];
									break;
								} else {
									if(i < argc - 1) {
										i++;
										p_opt->opt_value = argv[i];
									}
								}
							}
						}

						j++;
					}
				}
			}
		}
	}

	bool find_value(_cstr_t val, _cmd_opt_t *p_opt_array) {
		bool r = false;
		_u32 n = 0;

		while(p_opt_array[n].opt_name) {
			if(p_opt_array[n].opt_value) {
				if(mpi_str->str_cmp(val, p_opt_array[n].opt_value) == 0) {
					r = true;
					break;
				}
			}
			n++;
		}

		return r;
	}

	_cmd_t *get_cmd_info(_cstr_t cmd_name, iCmd **pi_cmd, _cmd_rec_t **pp_rec) {
		_cmd_rec_t *cmd_rec = find_command(cmd_name);
		_cmd_t *r = (cmd_rec) ? cmd_rec->pi_cmd->get_info() : 0;

		if(r) {
			_u32 n = 0;

			while(r[n].cmd_name) {
				if(mpi_str->str_cmp(cmd_name, (_str_t)r[n].cmd_name) == 0) {
					r += n;
					*pi_cmd = cmd_rec->pi_cmd;
					*pp_rec = cmd_rec;
					break;
				}
				n++;
			}

			if(r[n].cmd_name == 0)
				r = 0;
		}
		return r;
	}

	iCmd *mpi_cmd;

	BEGIN_LINK_MAP
		LINK(mpi_str, I_STR, NULL, RF_ORIGINAL, NULL, NULL),
		LINK(mpi_heap, I_HEAP, NULL, RF_ORIGINAL, NULL, NULL),
		LINK(mpi_cmd_map, I_MAP, NULL, RF_CLONE, NULL, NULL),
		LINK(mpi_cmd, I_CMD, NULL, RF_ORIGINAL|RF_CLONE|RF_PLUGIN, [](_u32 n, void *udata) {
			cCmdHost *p = (cCmdHost *)udata;

			switch(n) {
				case RCTL_REF:
					p->add_command(p->mpi_cmd);
					break;
				case RCTL_UNREF:
					p->remove_command(p->mpi_cmd);
					break;
			};
		}, this)
	END_LINK_MAP

public:
	BASE(cCmdHost, "cCmdHost", RF_ORIGINAL, 1,0,0);

	bool object_ctl(_u32 cmd, void *arg, ...) {
		bool r = false;

		switch(cmd) {
			case OCTL_INIT:
				if(mpi_cmd_map) {
					mpi_cmd_map->init(15);
					r = true;
				}
				break;
			case OCTL_UNINIT:
				r = true;
				break;
		}

		return r;
	}

	_cmd_t *get_info(_cstr_t cmd_name, iCmd **pi_cmd=0) {
		iCmd *_pi_cmd = 0;
		_cmd_rec_t *p_rec = 0;
		_cmd_t *r = get_cmd_info(cmd_name, &_pi_cmd, &p_rec);

		if(pi_cmd)
			*pi_cmd = _pi_cmd;
		return r;
	}

	void exec(_str_t cmd_line, iIO *pi_io) {
		_str_t cmd=0;
		_u32 cmd_len = mpi_str->str_len(cmd_line)+1;

		if(cmd_len > 1) {
			cmd_len++;
			if((cmd = (_str_t)mpi_heap->alloc(cmd_len))) {
				_str_t argv[MAX_ARGV];
				_u32 argc = 0;
				_cmd_t *p_cmd = 0;
				iCmd *pi_cmd = 0;
				_cmd_rec_t *p_rec = 0;

				mpi_str->mem_set(argv, 0, sizeof(argv));
				mpi_str->mem_set(cmd, 0, cmd_len);
				mpi_str->str_cpy(cmd, cmd_line, cmd_len);
				argc = parse_argv(cmd, cmd_len, argv);

				if((p_cmd = get_cmd_info(argv[0], &pi_cmd, &p_rec))) {
					if(p_cmd->cmd_handler) {
						_u32 noptions = num_options(p_cmd->cmd_options);
						_u32 mem_options = (noptions+1) * sizeof(_cmd_opt_t);
						_cmd_opt_t *p_opt = 0;

						if(noptions) {
							// copy options array
							if((p_opt = (_cmd_opt_t *)mpi_heap->alloc(mem_options))) {
								mpi_str->mem_cpy(p_opt, p_cmd->cmd_options, mem_options);
								parse_options(p_opt, argc, argv);
							}
						}

						p_rec->running = true;
						p_cmd->cmd_handler(pi_cmd, this, pi_io, p_opt, argc, (_cstr_t*)argv);
						p_rec->running = false;

						if(p_opt)
							mpi_heap->free(p_opt, mem_options);
					}
				} else {
					if(pi_io) {
						_str_t ucmd = (_str_t)"Unknown command !\n";
						pi_io->write(ucmd, mpi_str->str_len(ucmd));
					}
				}

				mpi_heap->free(cmd, cmd_len);
			}
		}
	}

	// check for option
	bool option_check(_cstr_t opt_name, _cmd_opt_t *opt_array) {
		bool r = false;
		_cmd_opt_t *p_opt = find_option(opt_name, opt_array);

		if(p_opt) {
			if(p_opt->opt_flags & OF_PRESENT)
				r = true;
		}

		return r;
	}
	// get option value
	_cstr_t option_value(_cstr_t opt_name, _cmd_opt_t *opt_array) {
		_str_t r = 0;
		_cmd_opt_t *p_opt = find_option(opt_name, opt_array);

		if(p_opt) {
			if(p_opt->opt_flags & OF_PRESENT)
				r = p_opt->opt_value;

			// check for environment variable
			if(r && r[0] == '$')
				r = getenv(&r[1]);
		}

		return r;
	}

	// retrieve arguments
	_cstr_t argument(_u32 argc, _cstr_t argv[], _cmd_opt_t *p_opt_array, _u32 idx) {
		_cstr_t r = 0;
		_u32 arg_idx = 0;

		for(_u32 i = 0; i < argc; i++) {
			_cstr_t arg = argv[i];

			if(arg[0] != '-') {
				if(find_value(arg, p_opt_array) == false) {
					if(arg_idx == idx) {
						r = arg;
						break;
					}

					arg_idx++;
				}
			}
		}

		// check for environment variable
		if(r && r[0] == '$')
			r = getenv(&r[1]);

		return r;
	}

	void enumerate(_cb_enum_cmd_t *pcb, void *udata) {
		typedef struct {
			_cb_enum_cmd_t *pcb;
			void *udata;
		}_enum_t;

		_enum_t e = {pcb, udata};

		mpi_cmd_map->enumerate([](void *data, _u32 size, void *udata)->_s32 {
			_enum_t *pe = (_enum_t *)udata;
			_cmd_rec_t *p_rec = (_cmd_rec_t *)data;

			pe->pcb(p_rec->pi_cmd->get_info(), pe->udata);
			return ENUM_NEXT;
		}, &e);
	}
};

static cCmdHost _g_cmd_host_;

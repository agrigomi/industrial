#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "startup.h"
#include "iRepository.h"
#include "iLog.h"
#include "iArgs.h"
#include "iNet.h"
#include "iMemory.h"
#include "iCmd.h"

IMPLEMENT_BASE_ARRAY("command_example", 100);

void log_listener(_u8 lmt, _str_t msg) {
	_char_t pref = '-';

	switch(lmt) {
		case LMT_TEXT: pref = 'T'; break;
		case LMT_INFO: pref = 'I'; break;
		case LMT_ERROR: pref = 'E'; break;
		case LMT_WARNING: pref = 'W';break;
	}
	printf("[%c] %s\n", pref, msg);
}

static _cstr_t extensions[] = {
	"libnet.so",
	"libcmd.so",
	0
};

_err_t main(int argc, char *argv[]) {
	_err_t r = init(argc, argv);

	if(r == ERR_NONE) {
		_u32 listen_port = 3000;
		iRepository *pi_repo = get_repository();
		iLog *pi_log = (iLog*)pi_repo->object_by_iname(I_LOG, RF_ORIGINAL);

		if(pi_log)
			pi_log->add_listener(log_listener);
		else {
			printf("Unable to create application log !\n");
			return 1;
		}

		// arguments
		iArgs *pi_arg = (iArgs *)pi_repo->object_by_iname(I_ARGS, RF_ORIGINAL);

		if(pi_arg) {
			_str_t port = pi_arg->value('p');

			if(port)
				listen_port = atoi(port);
			else
				pi_log->fwrite(LMT_WARNING, "Defauting listen port to %u", listen_port);

			// retrieve extensions directory from command line
			_str_t ext_dir = pi_arg->value("ext-dir");

			if(!ext_dir)
				// not found in command line, try from environment
				ext_dir = getenv("LD_LIBRARY_PATH");

			// set extensions directory
			pi_repo->extension_dir((ext_dir) ? ext_dir : "./");

			// no longer needed of arguments
			pi_repo->object_release(pi_arg);
		}

		// load extensions
		_u32 n = 0;

		while(extensions[n]) {
			_err_t err = pi_repo->extension_load(extensions[n]);

			if(err != ERR_NONE)
				pi_log->fwrite(LMT_ERROR, "Unable to load extension '%s'; err=%d", extensions[n], err);
			n++;
		}

		iNet *pi_net = (iNet *)pi_repo->object_by_iname(I_NET, RF_ORIGINAL);
		iLlist *pi_list = (iLlist *)pi_repo->object_by_iname(I_LLIST, RF_CLONE);
		iCmdHost *pi_cmd_host = (iCmdHost *)pi_repo->object_by_iname(I_CMD_HOST, RF_ORIGINAL);

		if(pi_net && pi_list && pi_cmd_host) {
			pi_list->init(LL_VECTOR, 1);
			pi_log->fwrite(LMT_INFO, "Create TCP server on port %u", listen_port);

			iTCPServer *pi_server = pi_net->create_tcp_server(listen_port);

			if(pi_server) {
				pi_server->blocking(false);

				typedef struct {
					bool prompt = true;
					iSocketIO *pi_io;
				}_client_t;

				for(;;) {
					_u32 sz = 0;
					iSocketIO *pi_io = pi_server->listen();

					if(pi_io) {
						pi_io->blocking(false); // non blocking I/O
						_client_t client = {true, pi_io};
						pi_list->add(&client, sizeof(_client_t));
						pi_log->fwrite(LMT_INFO, "Incoming connection 0x%x", pi_io);
					}

					_client_t *pc = (_client_t *)pi_list->first(&sz);

					while(pc) {
						_char_t buffer[1024]="";
						_u32 len = 0;

						if(pc->pi_io->alive() == false) {
							pi_server->close(pc->pi_io);
							pi_log->fwrite(LMT_INFO, "Close connection 0x%x", pc->pi_io);
							pi_list->del();
							pc = (_client_t *)pi_list->current(&sz);
							continue;
						}

						if(pc->prompt) {
							pc->pi_io->write((_str_t)"cmdex: ", 7);
							pc->prompt = false;
						}
						if((len = pc->pi_io->read(buffer, sizeof(buffer))) > 0) {
							pi_cmd_host->exec(buffer, pc->pi_io);
							pc->prompt = true;
						}
						pc = (_client_t *)pi_list->next(&sz);
					}

					usleep(10000);
				}
			}
		}
	}

	return r;
}

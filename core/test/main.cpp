#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include "startup.h"
#include "iRepository.h"
#include "iLog.h"
#include "iArgs.h"
#include "iFS.h"
#include "iNet.h"
#include "iCmd.h"
#include "iMemory.h"
#include "iHT.h"

IMPLEMENT_BASE_ARRAY("core_test", 1024);

_cstr_t g_body = "<!DOCTYPE HTML>\
<form action=\"upload_file\" method=\"post\" enctype=\"multipart/form-data\">\
Select image to upload:\
<input type=\"file\" name=\"fileToUpload\" id=\"fileToUpload\">\
<input type=\"submit\" value=\"Upload Image\" name=\"submit\">\
</form>";

_err_t main(int argc, char *argv[]) {
	_err_t r = init(argc, argv);
	if(r == ERR_NONE) {
		iRepository *pi_repo = get_repository();
		iLog *pi_log = dynamic_cast<iLog *>(pi_repo->object_by_iname(I_LOG, RF_ORIGINAL));

		//pi_log->add_listener(log_listener);
		pi_log->add_listener([](_u8 lmt, _str_t msg) {
			_char_t pref = '-';

			switch(lmt) {
				case LMT_TEXT: pref = 'T'; break;
				case LMT_INFO: pref = 'I'; break;
				case LMT_ERROR: pref = 'E'; break;
				case LMT_WARNING: pref = 'W';break;
			}
			printf("[%c] %s\n", pref, msg);
		});

		pi_repo->extension_dir("./bin/deploy/unix");
		pi_repo->extension_load("extht.so");
		pi_repo->extension_load("extfs.so");
		pi_repo->extension_load("extnet.so");

		iNet *pi_net = dynamic_cast<iNet*>(pi_repo->object_by_iname(I_NET, RF_ORIGINAL));
		if(pi_net) {
			iHttpServer *pi_http = pi_net->create_http_server(8080);
			if(pi_http) {
				pi_http->on_event(HTTP_ON_OPEN, [](iHttpConnection *pi_httpc, void *udata) {
					printf(">>> on_open: %p\n", pi_httpc);
				});
				pi_http->on_event(HTTP_ON_REQUEST, [](iHttpConnection *pi_httpc, void *udata) {
					_u8 method = pi_httpc->req_method();
					pi_httpc->res_var("var1", "alabala");

					if(method == HTTP_METHOD_GET) {
						printf(">>> on_request(GET '%s') %p\n", pi_httpc->req_uri(), pi_httpc);
						printf("User-Agent: '%s'\n", pi_httpc->req_var("User-Agent"));

						if(strcmp(pi_httpc->req_uri(), "/home") == 0) {
							pi_httpc->res_code(HTTPRC_OK);
							pi_httpc->res_content_len(strlen(g_body));
							pi_httpc->res_write((_u8 *)g_body, strlen(g_body));
						} else {
							pi_httpc->res_code(HTTPRC_NOT_FOUND);
							//_cstr_t r = "<!DOCTYPE HTML><body><p>Requested URI, Not found (404)</p></body>";
							//pi_httpc->res_content_len(strlen(r));
							//pi_httpc->res_write((_u8*)r, strlen(r));
						}
					} else if(method == HTTP_METHOD_POST) {
						_u32 sz = 0;
						_str_t ptr = (_str_t)pi_httpc->req_data(&sz);
						printf(">>> on_request(POST '%s' [%u bytes]) %p\n", pi_httpc->req_uri(), sz, pi_httpc);
						if(sz)
							fwrite(ptr, sz, 1, stdout);

						pi_httpc->res_code(HTTPRC_OK);
						pi_httpc->res_content_len(strlen(g_body));
						pi_httpc->res_write((_u8 *)g_body, strlen(g_body));
					}
				});
				pi_http->on_event(HTTP_ON_REQUEST_DATA, [](iHttpConnection *pi_httpc, void *udata) {
					_u32 sz=0;
					_u8 *ptr = pi_httpc->req_data(&sz);

					printf(">>> on_request_data(%u bytes): %p\n", sz, pi_httpc);
					if(ptr && sz)
						fwrite(ptr, sz, 1, stdout);
				});
				pi_http->on_event(HTTP_ON_RESPONSE_DATA, [](iHttpConnection *pi_httpc, void *udata) {
					printf(">>> on_response_data: %p\n", pi_httpc);
				});
				pi_http->on_event(HTTP_ON_ERROR, [](iHttpConnection *pi_httpc, void *udata) {
					printf(">>> on_error(%d): %p\n", pi_httpc->error_code(), pi_httpc);
				});
				pi_http->on_event(HTTP_ON_CLOSE, [](iHttpConnection *pi_httpc, void *udata) {
					printf(">>> on_close: %p\n", pi_httpc);
				});

				while(getchar() != 'q');
				//while(pi_http->is_running())
				//	usleep(10000);
				pi_repo->object_release(pi_http);
			}
			pi_repo->object_release(pi_net);
		}

		iFS *pi_fs = dynamic_cast<iFS*>(pi_repo->object_by_iname(I_FS, RF_ORIGINAL));
		if(pi_fs) {
			iFileIO *pi_fio = pi_fs->open("test.xml", O_RDONLY);
			if(pi_fio) {
				iXML *pi_xml = (iXML *)pi_repo->object_by_iname(I_XML, RF_ORIGINAL);
				if(pi_xml) {
					_str_t xml = (_str_t)pi_fio->map(MPF_READ);
					_ulong xlen = pi_fio->size();
					HTCONTEXT hc = pi_xml->create_context();
					if(pi_xml->parse(hc, xml, xlen)) {
						_u32 sz  = 0;

						HTTAG test = pi_xml->select(hc, "/t-1/test", NULL, 0);
						if(test) {
							_str_t test_content = pi_xml->content(test, &sz);
							fwrite(test_content, sz, 1, stdout);
						} else
							pi_log->write(LMT_ERROR, "XML: No tag 't-1/test' found\n");

					} else
						pi_log->write(LMT_ERROR, "XML parse error");
					pi_xml->destroy_context(hc);
					pi_repo->object_release(pi_xml);
				}

				pi_fs->close(pi_fio);
			}
			pi_repo->object_release(pi_fs);
		}

		uninit();
	}

	return r;
}

class cTest: public iBase {
private:
	iLog *mpi_log;
public:
	BASE(cTest, "cTest", RF_ORIGINAL, 1,0,0);

	bool object_ctl(_u32 cmd, void *arg, ...) {
		switch(cmd) {
			case OCTL_INIT: {
				iRepository *pi_repo = (iRepository*)arg;
				if((mpi_log = (iLog*)pi_repo->object_by_iname(I_LOG, RF_ORIGINAL)))
					mpi_log->write(LMT_INFO, "init cTest");
				pi_repo->monitoring_add(0, "iObj1", 0, this);
				pi_repo->monitoring_add(0, I_FS, 0, this);
				break;
			}

			case OCTL_UNINIT:
				break;

			case OCTL_NOTIFY: {
				_notification_t *pn = (_notification_t *)arg;
				mpi_log->fwrite(LMT_INFO, "catch notification flags=0x%02x; object=0x%x", pn->flags, pn->object);
				if(pn->flags & NF_REMOVE)
					_gpi_repo_->object_release(pn->object);
				break;
			}
		}
		return true;
	}
};

static cTest _g_object_;

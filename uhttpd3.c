#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <errno.h>

#include <evhtp.h>

#define DEFAULT_PORT	9300

static int print_out_key(evhtp_kv_t * kv, void * arg)
{
    printf("%s : %s\n", kv->key, kv->val);

    (void) arg;
    return 0;
}

static void
dump_request(evhtp_request_t *req)
{
    const char *cmdtype;

    switch (req->method) {
        case htp_method_GET: cmdtype = "GET"; break;
        case htp_method_POST: cmdtype = "POST"; break;
        case htp_method_HEAD: cmdtype = "HEAD"; break;
        case htp_method_PUT: cmdtype = "PUT"; break;
        case htp_method_DELETE: cmdtype = "DELETE"; break;
        case htp_method_OPTIONS: cmdtype = "OPTIONS"; break;
        case htp_method_TRACE: cmdtype = "TRACE"; break;
        case htp_method_CONNECT: cmdtype = "CONNECT"; break;
        case htp_method_PATCH: cmdtype = "PATCH"; break;
        default: cmdtype = "unknown"; break;
    }

    printf("Received a %s request for %s\n",
            cmdtype, req->uri->path->full);

    printf("Headers:\n");
    evhtp_headers_t *headers = req->headers_in;
    evhtp_kvs_for_each(headers, print_out_key, NULL);

    printf("Queries:\n");
    evhtp_query_t *queries = req->uri->query;
    evhtp_kvs_for_each(queries, print_out_key, NULL);
}

static void
default_cb(evhtp_request_t * req, void * arg)
{
    dump_request(req);
    evbuffer_add_printf(req->buffer_out, "%s", arg ? (const char *)arg : "Null");
    evhtp_send_reply(req, EVHTP_RES_OK);
}

static void
set_request_cb(evhtp_request_t * req, void * arg)
{
    dump_request(req);
    evbuffer_add_printf(req->buffer_out, "%s", arg ? (const char *)arg : "Null");
    evhtp_send_reply(req, EVHTP_RES_OK);
}

/* Callback used for the /get URI */
static void
get_request_cb(evhtp_request_t * req, void * arg)
{
    dump_request(req);
    evbuffer_add_printf(req->buffer_out, "%s", arg ? (const char *)arg : "Null");
    evhtp_send_reply(req, EVHTP_RES_OK);
}

int
main(int argc, char ** argv)
{
    int ret = 0;
    uint16_t port = DEFAULT_PORT;
    evbase_t *evbase = event_base_new();
    evhtp_t  *htp    = evhtp_new(evbase, NULL);

    if (argc == 2 && atoi(argv[1])) {
        port = atoi(argv[1]);
    }

    evhtp_set_cb(htp, "/", default_cb, "Hello WhosKPW3\n");
    evhtp_set_cb(htp, "/set", set_request_cb, "Set\n");
    evhtp_set_cb(htp, "/get", get_request_cb, "Get\n");
#ifndef EVHTP_DISABLE_EVTHR
    evhtp_use_threads(htp, NULL, 4, NULL);
#endif
    evhtp_bind_socket(htp, "0.0.0.0", port, 1024);

    event_base_loop(evbase, 0);

    evhtp_unbind_socket(htp);
    evhtp_free(htp);
    event_base_free(evbase);

    return ret;
}


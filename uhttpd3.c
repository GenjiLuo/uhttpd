#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <errno.h>

#include <evhtp.h>

#define DEFAULT_PORT	9300

static void
default_cb(evhtp_request_t * req, void *arg)
{
    evbuffer_add_printf(req->buffer_out, "%s", arg ? (const char *)arg : "Null");
    evhtp_send_reply(req, EVHTP_RES_OK);
}

static void
set_request_cb(evhtp_request_t *req, void *arg)
{
    evbuffer_add_printf(req->buffer_out, "%s", arg ? (const char *)arg : "Null");
    evhtp_send_reply(req, EVHTP_RES_OK);
}

/* Callback used for the /get URI */
static void
get_request_cb(evhtp_request_t *req, void *arg)
{
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

    evhtp_set_cb(htp, "/", default_cb, "Hello WhosKPW3");
    evhtp_set_cb(htp, "/set", set_request_cb, "Set");
    evhtp_set_cb(htp, "/get", get_request_cb, "Get");
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


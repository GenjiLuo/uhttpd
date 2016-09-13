#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <errno.h>
#include <unistd.h>

#include <evhtp.h>
#include <evthr.h>
#include <libmemcached/memcached.h>

#define DEFAULT_PORT	9600


const char * memcached_conf_str = "--SERVER=127.0.0.1";
static memcached_st * memc = NULL;

static int print_out_key(evhtp_kv_t * kv, void * arg)
{
    printf("%s : %s\n", kv->key, kv->val);

    (void) arg;
    return 0;
}

static void
dump_request(evhtp_request_t *req)
{
#if DEBUG
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

    printf("<<<\n");
    printf("Thread : %p\n", get_request_thr(req));
    printf("Request : %s\n", cmdtype);
    printf("URI : %s\n", req->uri->path->full);

    printf("Headers:\n");
    evhtp_headers_t *headers = req->headers_in;
    evhtp_kvs_for_each(headers, print_out_key, NULL);

    printf("Queries:\n");
    evhtp_query_t *queries = req->uri->query;
    evhtp_kvs_for_each(queries, print_out_key, NULL);
    printf(">>>\n");
#else
    (void) req;
    (void) print_out_key;
#endif
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

    const char * key = evhtp_kv_find(req->uri->query, "key");
    const char * val = evhtp_kv_find(req->uri->query, "value");
    if (key && val) {
        memcached_return_t rc= memcached_set(memc, key, strlen(key), val, strlen(val), (time_t)0, (uint32_t)0);
        if (rc != MEMCACHED_SUCCESS) {
            evbuffer_add_printf(req->buffer_out, "FAIL\n");
        } else {
            evbuffer_add_printf(req->buffer_out, "OK\n");
        }
    } else {
        evbuffer_add_printf(req->buffer_out,
                "Invalid parameters!Expect: /set?key=xxx&value=yyy\n"
                );
    }
    evhtp_send_reply(req, EVHTP_RES_OK);

    (void) arg;
}

/* Callback used for the /get URI */
static void
get_request_cb(evhtp_request_t * req, void * arg)
{
    dump_request(req);

    const char *key = evhtp_kv_find(req->uri->query, "key");

    if (key) {
        size_t val_len = 0;
        uint32_t flag = 0;
        memcached_return_t rc;
        char *val = memcached_get(memc, key, strlen(key), &val_len, &flag, &rc);
        if (!val || rc != MEMCACHED_SUCCESS) {
            evbuffer_add_printf(req->buffer_out, "NOT_FOUND\n");
        } else {
            evbuffer_add_printf(req->buffer_out,
                    "value=%s\n", val);
        }
    } else {
        evbuffer_add_printf(req->buffer_out,
                "Invalid parameters! Expect: /get?key=xxx\n"
                );
    }
    evhtp_send_reply(req, EVHTP_RES_OK);

    (void) arg;
}


int
main(int argc, char ** argv)
{
    int ret = 0;
    int c = -1;
    uint16_t port = DEFAULT_PORT;
    evbase_t *evbase = event_base_new();
    evhtp_t  *htp    = evhtp_new(evbase, NULL);

    while ((c = getopt(argc, argv, "p:")) != -1) {
        switch (c) {
            case 'p':
                port = atoi(optarg);
                break;
            default:
                fprintf(stderr,
                        "Usage: %s [-p port] [-t num_of_threads]\n",
                        argv[0]);
                exit(EXIT_FAILURE);
        }
    }

    memc = memcached(memcached_conf_str, strlen(memcached_conf_str));

    evhtp_set_gencb(htp, default_cb, NULL);
    evhtp_set_cb(htp, "/", default_cb, "Hello WhosKPW3\n");
    evhtp_set_cb(htp, "/set", set_request_cb, "Set\n");
    evhtp_set_cb(htp, "/get", get_request_cb, "Get\n");

    evhtp_bind_socket(htp, "0.0.0.0", port, 1024);

    event_base_loop(evbase, 0);

    // Free memcached
    memcached_free(memc);

    evhtp_unbind_socket(htp);
    evhtp_free(htp);
    event_base_free(evbase);

    return ret;
}


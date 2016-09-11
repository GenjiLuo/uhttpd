#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <errno.h>

#include <evhtp.h>
#include <evthr.h>
#include <hiredis/hiredis.h>
#include <hiredis/async.h>
#include <hiredis/adapters/libevent.h>

#define DEFAULT_PORT	9300
#define REDIS_HOST		"127.0.0.1"
#define REDIS_PORT		6379

struct app_parent {
    evhtp_t  * evhtp;
    evbase_t * evbase;
    char     * redis_host;
    uint16_t   redis_port;
};

struct app {
    struct app_parent * parent;
    evbase_t          * evbase;
    redisAsyncContext * redis;
};

static evthr_t *
get_request_thr(evhtp_request_t * req) {
    evhtp_connection_t * htpconn;
    evthr_t            * thread;

    htpconn = evhtp_request_get_connection(req);
    thread  = htpconn->thread;

    return thread;
}

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


void
app_process_request(evhtp_request_t * req, void * arg)
{
    struct sockaddr_in * sin;
    struct app_parent  * app_parent;
    struct app         * app;
    evthr_t            * thread;
    evhtp_connection_t * conn;
    char                 tmp[1024];

    thread = get_request_thr(req);
    printf("o Thread(%p)\n", thread);
    dump_request(req);

    conn   = evhtp_request_get_connection(req);
    app    = (struct app *)evthr_get_aux(thread);
    sin    = (struct sockaddr_in *)conn->saddr;

    evutil_inet_ntop(AF_INET, &sin->sin_addr, tmp, sizeof(tmp));

#if 0
    /* increment a global counter of hits on redis */
    redisAsyncCommand(app->redis, redis_global_incr_cb,
            (void *)req, "INCR requests:total");

    /* increment a counter for hits from this source address on redis */
    redisAsyncCommand(app->redis, redis_srcaddr_incr_cb,
            (void *)req, "INCR requests:ip:%s", tmp);

    /* add the source port of this req to a source-specific set */
    redisAsyncCommand(app->redis, redis_set_srcport_cb, (void *)req,
            "SADD requests:ip:%s:ports %d", tmp, ntohs(sin->sin_port));

    /* get all of the ports this source address has used */
    redisAsyncCommand(app->redis, redis_get_srcport_cb, (void *)req,
            "SMEMBERS requests:ip:%s:ports", tmp);
#endif

    /* pause the req processing */
    evhtp_request_pause(req);
}

void
app_init_thread(evhtp_t * htp, evthr_t * thread, void * arg)
{
    struct app_parent * app_parent;
    struct app        * app;

    app_parent  = (struct app_parent *)arg;
    app         = calloc(sizeof(struct app), 1);

    app->parent = app_parent;
    app->evbase = evthr_get_base(thread);
    app->redis  = redisAsyncConnect(app_parent->redis_host, app_parent->redis_port);

    redisLibeventAttach(app->redis, app->evbase);

    evthr_set_aux(thread, app);
}


int
main(int argc, char ** argv)
{
    int ret = 0;
    uint16_t port = DEFAULT_PORT;
    evbase_t *evbase = event_base_new();
    evhtp_t  *htp    = evhtp_new(evbase, NULL);
    struct app_parent * app_p = calloc(sizeof(struct app_parent), 1);

    if (argc == 2 && atoi(argv[1])) {
        port = atoi(argv[1]);
    }

    app_p->evhtp      = htp;
    app_p->evbase     = evbase;
    app_p->redis_host = REDIS_HOST;
    app_p->redis_port = REDIS_PORT;

    evhtp_set_gencb(htp, app_process_request, NULL);
    evhtp_set_cb(htp, "/", app_process_request, "Hello WhosKPW3\n");
#if 1
    //evhtp_set_cb(htp, "/", default_cb, "Hello WhosKPW3\n");
    evhtp_set_cb(htp, "/set", set_request_cb, "Set\n");
    evhtp_set_cb(htp, "/get", get_request_cb, "Get\n");
#endif

#ifndef EVHTP_DISABLE_EVTHR
    evhtp_use_threads(htp, app_init_thread, 4, app_p);
#endif
    evhtp_bind_socket(htp, "0.0.0.0", port, 1024);

    event_base_loop(evbase, 0);

    evhtp_unbind_socket(htp);
    evhtp_free(htp);
    event_base_free(evbase);

    return ret;
}


#include <sys/types.h>
#include <sys/select.h>
#include <sys/socket.h>

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <microhttpd.h>
#include <hiredis/hiredis.h>

#define USE_REDIS		1

#if DEBUG
#define LOG(fmt, arg...)		printf(fmt, ##arg)
#else
#define LOG(fmt, arg...)
#endif

#define DEFAULT_PORT    8888
#define REDIS_HOST      "127.0.0.1"
#define REDIS_PORT      6379

#if USE_REDIS
static redisContext *redis_context;
#else
static char value_to_set[512];
#endif
static char page_to_answer[2048];

int print_out_key (void *cls, enum MHD_ValueKind kind,
        const char *key, const char *value)
{
    (void) cls;
    (void) kind;

    printf ("%s: %s\n", key, value);

    return MHD_YES;
}

int answer_to_connection (void *cls, struct MHD_Connection *connection,
        const char *url,
        const char *method, const char *version,
        const char *upload_data,
        size_t *upload_data_size, void **con_cls)
{
    const char *page_default = "<html><body>Hello, WhosKPW3!</body></html>";
    const char *page = page_default;
    struct MHD_Response *response;
    int pos = 0;
    int ret = 0;

    (void) cls;
    (void) upload_data;
    (void) upload_data_size;
    (void) con_cls;

    printf ("-- Header --\n");
    MHD_get_connection_values (connection, MHD_HEADER_KIND, &print_out_key, NULL);

    printf ("-- Argument --\n");
    MHD_get_connection_values (connection, MHD_GET_ARGUMENT_KIND, &print_out_key, NULL);

    // Response according to WhosKPW3 requirement
    if (!strcmp(url, "/get")) {
        if (MHD_lookup_connection_value (connection, MHD_GET_ARGUMENT_KIND, "key")) {
#if USE_REDIS
            redisReply *reply = redisCommand(redis_context, "GET %s", MHD_lookup_connection_value (connection, MHD_GET_ARGUMENT_KIND, "key"));
            if (reply) {
                switch (reply->type) {
                    case REDIS_REPLY_STRING:
                        pos = snprintf(page_to_answer, sizeof(page_to_answer) - pos, "<html><body>");
                        pos = snprintf(page_to_answer + strlen(page_to_answer), sizeof(page_to_answer) - strlen(page_to_answer), "value=%s", reply->str);
                        pos = snprintf(page_to_answer + strlen(page_to_answer), sizeof(page_to_answer) - strlen(page_to_answer), "</html></body>");
                        page = page_to_answer;
                        break;
                    case REDIS_REPLY_NIL:
                        page = "<html><body>value=NO_SET</body></html>";
                        break;
                    case REDIS_REPLY_ARRAY:
                    case REDIS_REPLY_INTEGER:
                    case REDIS_REPLY_STATUS:
                    case REDIS_REPLY_ERROR:
                    default:
                        page = "<html><body>value=ERROR</body></html>";
                        break;
                }
                freeReplyObject(reply);
            } else {
                page = "<html><body>value=ERROR</body></html>";
            }
#else
            pos = snprintf(page_to_answer, sizeof(page_to_answer) - pos, "<html><body>");
            pos = snprintf(page_to_answer + strlen(page_to_answer), sizeof(page_to_answer) - strlen(page_to_answer), "value=%s", value_to_set);
            pos = snprintf(page_to_answer + strlen(page_to_answer), sizeof(page_to_answer) - strlen(page_to_answer), "</html></body>");
            page = page_to_answer;
#endif
        } else {
            page = "<html><body>Invalid parameters! Expect: /get?key=xxx</body></html>";
        }
    } else if (!strcmp(url, "/set")) {
        if (MHD_lookup_connection_value (connection, MHD_GET_ARGUMENT_KIND, "key") &&
                MHD_lookup_connection_value (connection, MHD_GET_ARGUMENT_KIND, "value")) {
#if USE_REDIS
            redisReply *reply = redisCommand(redis_context,"SET %s %s",
                    MHD_lookup_connection_value (connection, MHD_GET_ARGUMENT_KIND, "key"),
                    MHD_lookup_connection_value (connection, MHD_GET_ARGUMENT_KIND, "value")
                    );
            if (!reply) {
                page = "<html><body>FAIL</body></html>";
            } else {
                if (reply->type == REDIS_REPLY_STATUS &&
                        strcasecmp(reply->str,"OK") == 0) {
                    page = "<html><body>OK</body></html>";
                } else {
                    page = "<html><body>FAIL</body></html>";
                }
                freeReplyObject(reply);
            }
#else
            page = "<html><body>OK</body></html>";
            snprintf(value_to_set, sizeof(value_to_set), "%s",
                    MHD_lookup_connection_value (connection, MHD_GET_ARGUMENT_KIND, "value"));
            printf ("Set value=%s\n", value_to_set);
#endif
        } else {
            page = "<html><body>Invalid parameters!Expect: /set?key=xxx&value=yyy</body></html>";
        }
    } else {
        page = page_default;
    }

    printf ("New %s request for %s using version %s\n", method, url, version);
    response = MHD_create_response_from_buffer (strlen (page),
            (void*) page, MHD_RESPMEM_PERSISTENT);

    ret = MHD_queue_response (connection, MHD_HTTP_OK, response);
    MHD_destroy_response (response);

return ret;
}

int main (int argc, char ** argv)
{
    int ret = 0;
    struct MHD_Daemon *daemon = NULL;
    uint16_t port = DEFAULT_PORT;

    if (argc == 2 && atoi(argv[1])) {
        port = atoi(argv[1]);
    }

#if USE_REDIS
    redis_context = redisConnect(REDIS_HOST, REDIS_PORT);
    if (!redis_context) {
        fprintf(stderr, "Couldn't connect redis. Exiting.\n");
        goto error;
    }
    if (redis_context->err) {
        fprintf(stderr, "Connect redis fail: %s. Exiting.\n", redis_context->errstr);
        goto error;
    }
#else
    snprintf(value_to_set, sizeof(value_to_set), "NOT_SET");
#endif

    daemon = MHD_start_daemon (MHD_USE_SELECT_INTERNALLY, port, NULL, NULL,
            &answer_to_connection, NULL, MHD_OPTION_END);

    if (NULL == daemon) {
        goto error;
    }
    goto cleanup;

error:
    ret = 1;
cleanup:
    (void) getchar ();
#if USE_REDIS
    if (redis_context) {
        redisFree(redis_context);
    }
#endif
    if (daemon) {
        MHD_stop_daemon (daemon);
    }

    return ret;
}

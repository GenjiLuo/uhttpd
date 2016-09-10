#include <sys/types.h>
#include <sys/select.h>
#include <sys/socket.h>

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <microhttpd.h>

#define DEFAULT_PORT     8888

static char value_to_set[512];
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
            pos = snprintf(page_to_answer, sizeof(page_to_answer) - pos, "<html><body>");
            pos = snprintf(page_to_answer + strlen(page_to_answer), sizeof(page_to_answer) - strlen(page_to_answer), "value=%s", value_to_set);
            pos = snprintf(page_to_answer + strlen(page_to_answer), sizeof(page_to_answer) - strlen(page_to_answer), "</html></body>");
            page = page_to_answer;
        } else {
            page = "<html><body>Invalid parameters! Expect: /get?key=xxx</body></html>";
        }
    } else if (!strcmp(url, "/set")) {
        if (MHD_lookup_connection_value (connection, MHD_GET_ARGUMENT_KIND, "key") &&
                MHD_lookup_connection_value (connection, MHD_GET_ARGUMENT_KIND, "value")) {
            page = "<html><body>OK</body></html>";
            snprintf(value_to_set, sizeof(value_to_set), "%s",
                    MHD_lookup_connection_value (connection, MHD_GET_ARGUMENT_KIND, "value"));
            printf ("Set value=%s\n", value_to_set);
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
    struct MHD_Daemon *daemon;
    uint16_t port = DEFAULT_PORT;

    if (argc == 2 && atoi(argv[1])) {
        port = atoi(argv[1]);
    }

    snprintf(value_to_set, sizeof(value_to_set), "NOT_SET");

    daemon = MHD_start_daemon (MHD_USE_SELECT_INTERNALLY, port, NULL, NULL,
            &answer_to_connection, NULL, MHD_OPTION_END);

    if (NULL == daemon) {
        return 1;
    }

    (void) getchar ();
    MHD_stop_daemon (daemon);

    return 0;
}

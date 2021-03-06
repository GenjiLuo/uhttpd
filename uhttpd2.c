/**
 * A trivial static http webserver using Libevent's evhttp.
 *
 * This is not the best code in the world, and it does some fairly stupid stuff
 * that you would never want to do in a production webserver. Caveat hackor!
 *
 */

/* Compatibility for possible missing IPv6 declarations */
#include <evhttp.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <sys/types.h>
#include <sys/stat.h>

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>
#include <io.h>
#include <fcntl.h>
#ifndef S_ISDIR
#define S_ISDIR(x) (((x) & S_IFMT) == S_IFDIR)
#endif
#else
#include <sys/stat.h>
#include <sys/socket.h>
#include <sys/queue.h>
#include <signal.h>
#include <fcntl.h>
#include <unistd.h>
#include <dirent.h>
#endif

#include <event2/event.h>
#include <event2/http.h>
#include <event2/buffer.h>
#include <event2/util.h>
#include <event2/keyvalq_struct.h>

#ifdef EVENT__HAVE_NETINET_IN_H
#include <netinet/in.h>
# ifdef _XOPEN_SOURCE_EXTENDED
#  include <arpa/inet.h>
# endif
#endif

#include <hiredis/hiredis.h>

#ifdef _WIN32
#ifndef stat
#define stat _stat
#endif
#ifndef fstat
#define fstat _fstat
#endif
#ifndef open
#define open _open
#endif
#ifndef close
#define close _close
#endif
#ifndef O_RDONLY
#define O_RDONLY _O_RDONLY
#endif
#endif

#define USE_REDIS		1

#if DEBUG
#define LOG(fmt, arg...)		printf(fmt, ##arg)
#else
#define LOG(fmt, arg...)
#endif

#define DEFAULT_PORT	9200
#define REDIS_HOST		"127.0.0.1"
#define REDIS_PORT		6379

#if USE_REDIS
static redisContext *redis_context = NULL;
#else
static char value_to_set[512];
#endif

static char uri_root[512];

static const struct table_entry {
	const char *extension;
	const char *content_type;
} content_type_table[] = {
	{ "txt", "text/plain" },
	{ "c", "text/plain" },
	{ "h", "text/plain" },
	{ "html", "text/html" },
	{ "htm", "text/htm" },
	{ "css", "text/css" },
	{ "gif", "image/gif" },
	{ "jpg", "image/jpeg" },
	{ "jpeg", "image/jpeg" },
	{ "png", "image/png" },
	{ "pdf", "application/pdf" },
	{ "ps", "application/postsript" },
	{ NULL, NULL },
};

/* Try to guess a good content-type for 'path' */
static const char *
guess_content_type(const char *path)
{
	const char *last_period, *extension;
	const struct table_entry *ent;
	last_period = strrchr(path, '.');
	if (!last_period || strchr(last_period, '/'))
		goto not_found; /* no exension */
	extension = last_period + 1;
	for (ent = &content_type_table[0]; ent->extension; ++ent) {
		if (!evutil_ascii_strcasecmp(ent->extension, extension))
			return ent->content_type;
	}

not_found:
	return "application/misc";
}

static void
dump_request(struct evhttp_request *req)
{
	const char *cmdtype;
	struct evkeyvalq *headers;
	struct evkeyval *header;
	struct evbuffer *buf;

	switch (evhttp_request_get_command(req)) {
		case EVHTTP_REQ_GET: cmdtype = "GET"; break;
		case EVHTTP_REQ_POST: cmdtype = "POST"; break;
		case EVHTTP_REQ_HEAD: cmdtype = "HEAD"; break;
		case EVHTTP_REQ_PUT: cmdtype = "PUT"; break;
		case EVHTTP_REQ_DELETE: cmdtype = "DELETE"; break;
		case EVHTTP_REQ_OPTIONS: cmdtype = "OPTIONS"; break;
		case EVHTTP_REQ_TRACE: cmdtype = "TRACE"; break;
		case EVHTTP_REQ_CONNECT: cmdtype = "CONNECT"; break;
		case EVHTTP_REQ_PATCH: cmdtype = "PATCH"; break;
		default: cmdtype = "unknown"; break;
	}

	printf("Received a %s request for %s\nHeaders:\n",
			cmdtype, evhttp_request_get_uri(req));

	headers = evhttp_request_get_input_headers(req);
	for (header = headers->tqh_first; header;
			header = header->next.tqe_next) {
		printf("  %s: %s\n", header->key, header->value);
	}

	buf = evhttp_request_get_input_buffer(req);
	puts("Input data: <<<");
	while (evbuffer_get_length(buf)) {
		int n;
		char cbuf[128];
		n = evbuffer_remove(buf, cbuf, sizeof(cbuf));
		if (n > 0)
			(void) fwrite(cbuf, 1, n, stdout);
	}
	puts(">>>");
}

/* Callback used for the /dump URI, and for every non-GET request:
 * dumps all information to stdout and gives back a trivial 200 ok */
static void
dump_request_cb(struct evhttp_request *req, void *arg)
{
	(void) arg;

	dump_request(req);

	evhttp_send_reply(req, 200, "OK", NULL);
}


/* Callback used for the /set URI */
static void
set_request_cb(struct evhttp_request *req, void *arg)
{
	struct evbuffer *evb = NULL;
	struct evkeyvalq header;
	struct evkeyval *kv;

	(void) arg;

#if DEBUG
	dump_request(req);
#endif

	TAILQ_INIT(&header);
	evhttp_parse_query(evhttp_request_get_uri(req), &header);
	TAILQ_FOREACH(kv, &header, next) {
		LOG("%s: %s\n", kv->key, kv->value);
	}

	/* equal to foreach as above */
	// kv = header->tqh_first;
	// while (kv) {
	// 	printf("%s: %s\n", kv->key, kv->value);
	// 	kv = kv->next.tqe_next;
	// }

	evb = evbuffer_new();

	const char *key = evhttp_find_header(&header, "key");
	const char *val = evhttp_find_header(&header, "value");
	if (key && val) {
#if USE_REDIS
		 redisReply *reply = redisCommand(redis_context,"SET %s %s", key, val);
		 if (!reply) {
             evbuffer_add_printf(evb,
                     "FAIL\n"
                     );
		 } else {
             if (reply->type == REDIS_REPLY_STATUS &&
                         strcasecmp(reply->str,"OK") == 0) {
                 evbuffer_add_printf(evb,
                         "OK\n"
                         );
             } else {
                 evbuffer_add_printf(evb,
                         "FAIL\n"
                         );
             }
             freeReplyObject(reply);
         }
#else
		snprintf(value_to_set, sizeof(value_to_set), "%s", val);
		LOG ("Set value=%s\n", value_to_set);
#endif
	} else {
		evbuffer_add_printf(evb,
				"Invalid parameters!Expect: /set?key=xxx&value=yyy"
				);
	}

	evhttp_send_reply(req, 200, "OK", evb);
}

/* Callback used for the /get URI */
static void
get_request_cb(struct evhttp_request *req, void *arg)
{
	struct evbuffer *evb = NULL;
	struct evkeyvalq header;
	struct evkeyval *kv;

	(void) arg;

#if DEBUG
	dump_request(req);
#endif

	TAILQ_INIT(&header);
	evhttp_parse_query(evhttp_request_get_uri(req), &header);
	TAILQ_FOREACH(kv, &header, next) {
		LOG("%s: %s\n", kv->key, kv->value);
	}

	evb = evbuffer_new();

	const char *key = evhttp_find_header(&header, "key");
	if (key) {
#if USE_REDIS
		redisReply *reply = redisCommand(redis_context, "GET %s", key);
		if (reply) {
			switch (reply->type) {
				case REDIS_REPLY_STRING:
					evbuffer_add_printf(evb,
							"value=%s\n",
							reply->str
							);
					break;
				case REDIS_REPLY_NIL:
					evbuffer_add_printf(evb,
							"value=NO_SET\n"
							);
					break;
				case REDIS_REPLY_ARRAY:
				case REDIS_REPLY_INTEGER:
				case REDIS_REPLY_STATUS:
				case REDIS_REPLY_ERROR:
				default:
					evbuffer_add_printf(evb,
							"value=ERROR\n"
							);
					break;
			}
			freeReplyObject(reply);
		} else {
			evbuffer_add_printf(evb,
					"value=ERROR\n"
					);
		}
#else
		evbuffer_add_printf(evb,
				"value=%s\n",
				value_to_set
				);
#endif
	} else {
		evbuffer_add_printf(evb,
				"Invalid parameters! Expect: /get?key=xxx"
				);
	}

	evhttp_send_reply(req, 200, "OK", evb);
}


/* This callback gets invoked when we get any http request that doesn't match
 * any other callback.  Like any evhttp server callback, it has a simple job:
 * it must eventually call evhttp_send_error() or evhttp_send_reply().
 */
static void
send_document_cb(struct evhttp_request *req, void *arg)
{
	struct evbuffer *evb = NULL;
	const char *docroot = arg;
	const char *uri = evhttp_request_get_uri(req);
	struct evhttp_uri *decoded = NULL;
	const char *path;
	char *decoded_path;
	char *whole_path = NULL;
	size_t len;
	int fd = -1;
	struct stat st;

	if (evhttp_request_get_command(req) != EVHTTP_REQ_GET) {
		dump_request_cb(req, arg);
		return;
	}

	LOG("Got a GET request for <%s>\n",  uri);

	/* Decode the URI */
	decoded = evhttp_uri_parse(uri);
	if (!decoded) {
		LOG("It's not a good URI. Sending BADREQUEST\n");
		evhttp_send_error(req, HTTP_BADREQUEST, 0);
		return;
	}

	/* Let's see what path the user asked for. */
	path = evhttp_uri_get_path(decoded);
	if (!path) path = "/";

	/* We need to decode it, to see what path the user really wanted. */
	decoded_path = evhttp_uridecode(path, 0, NULL);
	if (decoded_path == NULL)
		goto err;
	/* Don't allow any ".."s in the path, to avoid exposing stuff outside
	 * of the docroot.  This test is both overzealous and underzealous:
	 * it forbids aceptable paths like "/this/one..here", but it doesn't
	 * do anything to prevent symlink following." */
	if (strstr(decoded_path, ".."))
		goto err;

	len = strlen(decoded_path)+strlen(docroot)+2;
	if (!(whole_path = malloc(len))) {
		perror("malloc");
		goto err;
	}
	evutil_snprintf(whole_path, len, "%s/%s", docroot, decoded_path);

	if (stat(whole_path, &st)<0) {
		goto err;
	}

	/* This holds the content we're sending. */
	evb = evbuffer_new();

	if (S_ISDIR(st.st_mode)) {
		/* If it's a directory, read the comments and make a little
		 * index page */
#ifdef _WIN32
		HANDLE d;
		WIN32_FIND_DATAA ent;
		char *pattern;
		size_t dirlen;
#else
		DIR *d;
		struct dirent *ent;
#endif
		const char *trailing_slash = "";

		if (!strlen(path) || path[strlen(path)-1] != '/')
			trailing_slash = "/";

#ifdef _WIN32
		dirlen = strlen(whole_path);
		pattern = malloc(dirlen+3);
		memcpy(pattern, whole_path, dirlen);
		pattern[dirlen] = '\\';
		pattern[dirlen+1] = '*';
		pattern[dirlen+2] = '\0';
		d = FindFirstFileA(pattern, &ent);
		free(pattern);
		if (d == INVALID_HANDLE_VALUE)
			goto err;
#else
		if (!(d = opendir(whole_path)))
			goto err;
#endif

		evbuffer_add_printf(evb,
				"<!DOCTYPE html>\n"
				"<html>\n <head>\n"
				"  <meta charset='utf-8'>\n"
				"  <title>%s</title>\n"
				"  <base href='%s%s'>\n"
				" </head>\n"
				" <body>\n"
				"  <h1>%s</h1>\n"
				"  <ul>\n",
				decoded_path, /* XXX html-escape this. */
				path, /* XXX html-escape this? */
				trailing_slash,
				decoded_path /* XXX html-escape this */);
#ifdef _WIN32
		do {
			const char *name = ent.cFileName;
#else
			while ((ent = readdir(d))) {
				const char *name = ent->d_name;
#endif
				evbuffer_add_printf(evb,
						"    <li><a href=\"%s\">%s</a>\n",
						name, name);/* XXX escape this */
#ifdef _WIN32
			} while (FindNextFileA(d, &ent));
#else
		}
#endif
		evbuffer_add_printf(evb, "</ul></body></html>\n");
#ifdef _WIN32
		FindClose(d);
#else
		closedir(d);
#endif
		evhttp_add_header(evhttp_request_get_output_headers(req),
				"Content-Type", "text/html");
	} else {
		/* Otherwise it's a file; add it to the buffer to get
		 * sent via sendfile */
		const char *type = guess_content_type(decoded_path);
		if ((fd = open(whole_path, O_RDONLY)) < 0) {
			perror("open");
			goto err;
		}

		if (fstat(fd, &st)<0) {
			/* Make sure the length still matches, now that we
			 * opened the file :/ */
			perror("fstat");
			goto err;
		}
		evhttp_add_header(evhttp_request_get_output_headers(req),
				"Content-Type", type);
		evbuffer_add_file(evb, fd, 0, st.st_size);
	}

	evhttp_send_reply(req, 200, "OK", evb);
	goto done;
err:
	evhttp_send_error(req, 404, "Document was not found");
	if (fd>=0)
		close(fd);
done:
	if (decoded)
		evhttp_uri_free(decoded);
	if (decoded_path)
		free(decoded_path);
	if (whole_path)
		free(whole_path);
	if (evb)
		evbuffer_free(evb);
}

int
main(int argc, char **argv)
{
	int ret = 0;
	int c = -1;
	unsigned short port = DEFAULT_PORT;
	char *root_dir = ".";

	struct event_base *base = NULL;
	struct evhttp *http = NULL;
	struct evhttp_bound_socket *handle = NULL;

#ifdef _WIN32
	WSADATA WSAData;
	WSAStartup(0x101, &WSAData);
#else
	if (signal(SIGPIPE, SIG_IGN) == SIG_ERR)
		goto error;
#endif

	while ((c = getopt(argc, argv, "p:f:")) != -1) {
		switch (c) {
			case 'p':
				port = atoi(optarg);
				break;
			case 'f':
				root_dir = optarg;
				break;
			default:
				fprintf(stderr,
						"Usage: %s [-p port] [-f root_dir]\n",
						argv[0]);
				exit(EXIT_FAILURE);
		}
	}


	base = event_base_new();
	if (!base) {
		fprintf(stderr, "Couldn't create an event_base: exiting\n");
		goto error;
	}

	/* Create a new evhttp object to handle requests. */
	http = evhttp_new(base);
	if (!http) {
		fprintf(stderr, "Couldn't create evhttp. Exiting.\n");
		goto error;
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

	/* The /dump URI will dump all requests to stdout and say 200 ok. */
	evhttp_set_cb(http, "/dump", dump_request_cb, NULL);

	evhttp_set_cb(http, "/set", set_request_cb, NULL);
	evhttp_set_cb(http, "/get", get_request_cb, NULL);

	/* We want to accept arbitrary requests, so we need to set a "generic"
	 * cb.  We can also add callbacks for specific paths. */
	evhttp_set_gencb(http, send_document_cb, root_dir);

	/* Now we tell the evhttp what port to listen on */
	handle = evhttp_bind_socket_with_handle(http, "0.0.0.0", port);
	if (!handle) {
		fprintf(stderr, "couldn't bind to port %d. Exiting.\n",
				(int)port);
		goto error;
	}

	{
		/* Extract and display the address we're listening on. */
		struct sockaddr_storage ss;
		evutil_socket_t fd;
		ev_socklen_t socklen = sizeof(ss);
		char addrbuf[128];
		void *inaddr;
		const char *addr;
		int got_port = -1;
		fd = evhttp_bound_socket_get_fd(handle);
		memset(&ss, 0, sizeof(ss));
		if (getsockname(fd, (struct sockaddr *)&ss, &socklen)) {
			perror("getsockname() failed");
			goto error;
		}
		if (ss.ss_family == AF_INET) {
			got_port = ntohs(((struct sockaddr_in*)&ss)->sin_port);
			inaddr = &((struct sockaddr_in*)&ss)->sin_addr;
		} else if (ss.ss_family == AF_INET6) {
			got_port = ntohs(((struct sockaddr_in6*)&ss)->sin6_port);
			inaddr = &((struct sockaddr_in6*)&ss)->sin6_addr;
		} else {
			fprintf(stderr, "Weird address family %d\n",
					ss.ss_family);
			goto error;
		}
		addr = evutil_inet_ntop(ss.ss_family, inaddr, addrbuf,
				sizeof(addrbuf));
		if (addr) {
			fprintf(stdout, "Listening on %s:%d\n", addr, got_port);
			evutil_snprintf(uri_root, sizeof(uri_root),
					"http://%s:%d",addr,got_port);
		} else {
			fprintf(stderr, "evutil_inet_ntop failed\n");
			goto error;
		}
	}

	event_base_dispatch(base);
	goto cleanup;

error:
	ret = 1;
cleanup:
	if (handle) {
	}
#if USE_REDIS
	if (redis_context) {
		redisFree(redis_context);
	}
#endif
	if (http) {
		evhttp_free(http);
	}
	if (base) {
		event_base_free(base);
	}

	return ret;
}

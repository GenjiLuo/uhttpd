# uhttpd

Programming practise for WhosKPW3.
- uhttpd  - implemented with `libmicrohttpd`.
- uhttpd2 - implemented with `libevent`.

## Setup

```
$ sudo apt-get install libmicrohttpd-dev libevent-dev
```

## Build

```
$ cc uhttpd.c -o uhttpd -Wall -W -O2 -I/usr/include -L /usr/lib/x86_64-linux-gnu/ -lmicrohttpd
$ cc uhttpd2.c -o uhttpd2 -Wall -W -O2 -I/usr/include -L /usr/lib/x86_64-linux-gnu/ -levent
-or-
$ make clean && make
```

## Test

```
# server
$ ./uhttpd [port] (default 8888)
-or-
$ ./uhttpd2 path/to/page [port] (default 9000)

# client
$ curl http://host_ip_or_name:8888
$ curl http://host_ip_or_name:8888/set?key=abc&value=1234
$ curl http://host_ip_or_name:8888/get?key=abc
```

## Reference

- https://www.gnu.org/software/libmicrohttpd/manual/libmicrohttpd.html
- https://www.gnu.org/software/libmicrohttpd/tutorial.pdf
- http://www.wangafu.net/~nickm/libevent-2.1/doxygen/html/http_8h.html
- http://www.wangafu.net/~nickm/libevent-book/
- http://www.man7.org/linux/man-pages/man3/queue.3.html

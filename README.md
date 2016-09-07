# uhttpd

A micro httpd with `libmicrohttpd` for WhosKPW3

## Setup

```
$ sudo apt-get install libmicrohttpd-dev
```

## Build

```
$ cc uhttpd.c -o uhttpd -I/usr/include -L /usr/lib/x86_64-linux-gnu/ -lmicrohttpd
-or-
$ make clean && make
```

## Test

```
# server
$ ./uhttpd [port] (port default 8888)

# client
$ curl http://host_ip_or_name:8888
$ curl http://host_ip_or_name:8888/set?key=abc&value=1234
$ curl http://host_ip_or_name:8888/get?key=abc
```

## Reference

- https://www.gnu.org/software/libmicrohttpd/manual/libmicrohttpd.html
- https://www.gnu.org/software/libmicrohttpd/tutorial.pdf

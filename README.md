# uhttpd

An micro httpd with libmicrohttpd for WhosKPW3

## Setup

```
$ sudo apt-get install libmicrohttpd-dev
```

## Build

```
$ cc uhttpd.c -o uhttpd -I/usr/include -L /usr/lib/x86_64-linux-gnu/ -lmicrohttpd
```

## Test

```
# server
$ ./uhttpd

# client
$ curl http://host_ip_or_name:8888
$ curl http://host_ip_or_name:8888/get?key=abc
$ curl http://host_ip_or_name:8888/set?key=abc&value=1234
```

## Reference

- https://www.gnu.org/software/libmicrohttpd/manual/libmicrohttpd.html
- https://www.gnu.org/software/libmicrohttpd/tutorial.pdf

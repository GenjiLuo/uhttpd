# uhttpd

Programming practise for [WhosKPW3](https://github.com/xilp/muapub/wiki/WhosKPW3).
- uhttpd  - implemented with `libmicrohttpd` + `redis`.
- uhttpd2 - implemented with `libevent` + `redis`.
- uhttpd3 - implemented with `libevent` + `libevhtp` + `redis` in multi-thread and async i/o.

## Setup

### libmicrohttpd

```
$ sudo apt-get install libmicrohttpd-dev
```

### libevent

```
$ sudo apt-get install libevent-dev
```

### libevhtp

```
$ wget https://github.com/ellzey/libevhtp/archive/1.2.9.tar.gz
$ tar zxvf 1.2.9.tar.gz
$ cd libevhtp-1.2.9
$ mkdir build && cd build
$ cmake .. -DEVHTP_BUILD_SHARED:STRING=ON
$ make
$ sudo make install
$ sudo ldconfig
```

### hiredis

```
$ wget https://github.com/redis/hiredis/archive/v0.13.3.tar.gz
$ tar zxvf v0.13.3.tar.gz
$ cd hiredis-0.13.3
$ make
$ sudo make install
$ sudo ldconfig
```

**Remember to run `sudo ldconfig` to update `/etc/ld.so.cache`, or you may encouter following error when run target:**

```
error while loading shared libraries: libhiredis.so.0.13: cannot open shared object file: No such file or directory
```

### redis

#### Installing Redis

```
$ sudo apt-get install build-essential tcl8.5
$ wget http://download.redis.io/releases/redis-stable.tar.gz
$ tar xzf redis-stable.tar.gz
$ cd redis-stable
$ make
$ make test
$ sudo make install
```

#### Installing Redis Service

```
$ cd redis-stable/utils
$ sudo ./install_server.sh
```

#### Start Redis

You can start and stop redis with these commands (the number depends on the port you set during the installation. 6379 is the default port setting):

```
$ sudo service redis_6379 start
$ sudo service redis_6379 stop
```

You can then access the redis database by typing the following command:

```
$ redis-cli
```

You now have Redis installed and running. The prompt will look like this:

```
redis 127.0.0.1:6379> 
```

To set Redis to automatically start at boot, run:

```
$ sudo update-rc.d redis_6379 defaults
```

> http://redis.io/topics/quickstart

> https://www.digitalocean.com/community/tutorials/how-to-install-and-use-redis

## Build

```
$ cc uhttpd.c -o uhttpd -Wall -W -O2 -I/usr/include -I/usr/local/include -L /usr/lib/x86_64-linux-gnu/ -L /usr/local/lib -lmicrohttpd -lhiredis
$ cc uhttpd2.c -o uhttpd2 -Wall -W -O2 -I/usr/include -I/usr/local/include -L /usr/lib/x86_64-linux-gnu/ -L /usr/local/lib -levent -lhiredis
$ cc uhttpd3.c -o uhttpd3 -Wall -W -O2 -I/usr/include -I/usr/local/include -L /usr/lib/x86_64-linux-gnu/ -L /usr/local/lib -levent -levhtp -lhiredis
```

Or,

```
$ make clean && make
```

To enable DEBUG:

```
$ DEBUG=1 make
```

## Test

```
# server
$ ./uhttpd [-p port] (default 9100)
-or-
$ ./uhttpd2 [-p port] [-f root_dir] (default port 9200 and root_dir '.')
-or-
$ ./uhttpd3 [-p port] [-t num_of_threads] (default port 9300 and num_of_threads 4)

# client
$ curl http://host_ip_or_name:9x00
$ curl http://host_ip_or_name:9x00/set?key=abc&value=1234
$ curl http://host_ip_or_name:9x00/get?key=abc
```

## Reference

- https://www.gnu.org/software/libmicrohttpd/manual/libmicrohttpd.html
- https://www.gnu.org/software/libmicrohttpd/tutorial.pdf
- http://www.wangafu.net/~nickm/libevent-2.1/doxygen/html/http_8h.html
- http://www.wangafu.net/~nickm/libevent-book/
- http://www.man7.org/linux/man-pages/man3/queue.3.html

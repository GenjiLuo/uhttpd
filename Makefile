all: target
FORCE:
	;

.PHONY: FORCE

target: uhttpd uhttpd2

CC=cc

%.o: %.c
	@$(CC) $(CFLAGS) -c $< -o $@

uhttpd: uhttpd.c
	$(CC) uhttpd.c -o uhttpd -Wall -W -O2 -I/usr/include -L /usr/lib/x86_64-linux-gnu/ -lmicrohttpd

uhttpd2: uhttpd2.c
	$(CC) uhttpd2.c -o uhttpd2 -Wall -W -O2 -I/usr/include -L /usr/lib/x86_64-linux-gnu/ -levent

clean:
	rm -f *.o uhttpd uhttpd2

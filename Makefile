all: target
FORCE:
	;

.PHONY: FORCE

target: uhttpd uhttpd2

CC=cc

CFLAGS=-Wall -W -O2 -I/usr/include -I/usr/local/include

LDFLAGS=-L /usr/lib/x86_64-linux-gnu/ -L /usr/local/lib -lmicrohttpd -levent -lhiredis

%.o: %.c
	@$(CC) $(CFLAGS) -c $< -o $@

uhttpd: uhttpd.c
	$(CC) $< -o $@ $(CFLAGS) $(LDFLAGS)

uhttpd2: uhttpd2.c
	$(CC) $< -o $@ $(CFLAGS) $(LDFLAGS)

clean:
	rm -f *.o uhttpd uhttpd2

all: target
FORCE:
	;

.PHONY: FORCE

target: uhttpd uhttpd2

CC=cc

%.o: %.c
	@$(CC) $(CFLAGS) -c $< -o $@

uhttpd: 
	$(CC) uhttpd.c -o uhttpd -I/usr/include -L /usr/lib/x86_64-linux-gnu/ -lmicrohttpd

uhttpd2: 
	$(CC) uhttpd2.c -o uhttpd2 -I/usr/include -L /usr/lib/x86_64-linux-gnu/ -levent

clean:
	rm -f *.o uhttpd uhttpd2

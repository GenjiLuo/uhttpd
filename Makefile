all: target
FORCE:
	;

.PHONY: FORCE

target: uhttpd

CC=cc

%.o: %.c
	@$(CC) $(CFLAGS) -c $< -o $@

uhttpd: 
	$(CC) uhttpd.c -o uhttpd -I/usr/include -L /usr/lib/x86_64-linux-gnu/ -lmicrohttpd

clean:
	rm -f *.o uhttpd

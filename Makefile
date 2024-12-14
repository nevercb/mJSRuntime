CC = gcc
CFLAGS = -Iquickjs
LDFLAGS = quickjs/libquickjs.a -lm -ldl

all: runtime

runtime: main.c require.c module_cache.c console.c
	$(CC) main.c require.c module_cache.c console.c $(CFLAGS) $(LDFLAGS) -o runtime

clean:
	rm -f runtime
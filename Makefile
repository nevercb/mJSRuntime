CC = gcc
CFLAGS = -Iquickjs
LDFLAGS = quickjs/libquickjs.a -lm -ldl

all: runtime

runtime: main.c
	$(CC) main.c $(CFLAGS) $(LDFLAGS) -o runtime
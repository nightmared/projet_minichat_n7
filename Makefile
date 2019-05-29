CFLAGS = -std=c11 -g -Wall -pedantic

all: minichat-client minichat-server minichatv2

minichat-client: minichat-client.o
	gcc $(CFLAGS) $(LDFLAGS) -o $@ $^

minichat-server: minichat-server.o
	gcc $(CFLAGS) $(LDFLAGS) -o $@ $^

minichatv2: minichatv2.o
	gcc $(CFLAGS) $(LDFLAGS) -o $@ $^

%.o: %.c common.h
	gcc $(CFLAGS) -c $< -o $@

OBJECTS = $(FILES:.c=.o)
CFLAGS = -std=c11 -g -Wall -pedantic

all: minichat-client minichat-server

minichat-client: minichat-client.o $(OBJECTS)
	gcc $(CFLAGS) $(LDFLAGS) -o $@ $^

minichat-server: minichat-server.o $(OBJECTS)
	gcc $(CFLAGS) $(LDFLAGS) -o $@ $^

%.o: %.c common.h
	gcc $(CFLAGS) -c $< -o $@

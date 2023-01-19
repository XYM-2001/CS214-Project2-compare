CC=gcc
CFLAGS=-Wall -Werror -fsanitize=address

all:
	$(CC) $(CFLAGS) compare.c -o compare -lpthread -lm

clean:
	rm -rf ww

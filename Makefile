.POSIX:
CC      = cc
CFLAGS  = -std=c99 -Wall -Wextra -ggdb3 -O3 \
    -fsanitize=address -fsanitize=undefined
LDFLAGS =
LDLIBS  =

tests: tests.c trie.c trie.h
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ tests.c trie.c $(LDLIBS)

check: tests
	./tests

clean:
	rm -f tests

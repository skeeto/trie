CFLAGS = -std=c99 -Wall -g3 -O3

test : test.o trie.o

run : test
	./$^ xyloside < /usr/share/dict/american-english-insane

clean :
	$(RM) test *.o

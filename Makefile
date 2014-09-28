CFLAGS = -std=c99 -Wall -g3 -O3

test : test.o trie.o intern.o

run : test
	./$^ xylotile < /usr/share/dict/american-english-insane

clean :
	$(RM) test *.o

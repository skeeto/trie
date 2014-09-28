#ifndef NP_TRIE_H
#define NP_TRIE_H

typedef struct trie trie_t;

trie_t *trie_create();

void *trie_search(const trie_t *trie, const char *string);

int trie_insert(trie_t *trie, const char *string, void *data);

size_t trie_count(trie_t *trie);

#endif

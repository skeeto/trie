#ifndef NP_TRIE_H
#define NP_TRIE_H

typedef struct trie trie_t;

typedef int (*trie_visitor_t)(const char *key, void *data, void *arg);

trie_t *
trie_create();

void *
trie_search(const trie_t *trie, const char *string);

int
trie_insert(trie_t *trie, const char *string, void *data);

int
trie_visit(trie_t *trie, const char *prefix, trie_visitor_t visitor, void *arg);

size_t
trie_count(trie_t *trie, const char *prefix);

#endif

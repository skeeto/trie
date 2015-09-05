# C99 Trie Library

This is a fast, efficient [trie](https://en.wikipedia.org/wiki/Trie)
implementation. It serves well as a string-key hash table. See
`trie.h` for complete API documentation.

~~~c
trie * trie_create(void);
int    trie_free(trie *trie);
void * trie_search(const trie *trie, const char *key);
int    trie_insert(trie *trie, const char *key, void *data);
int    trie_replace(trie *trie, const char *key, trie_replacer f, void *arg);
int    trie_visit(trie *trie, const char *prefix, trie_visitor v, void *arg);
size_t trie_count(trie *trie, const char *prefix);
size_t trie_size(trie *trie);
~~~

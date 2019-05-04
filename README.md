# C99 Trie Library

This is a fast, efficient [trie](https://en.wikipedia.org/wiki/Trie)
implementation. It serves well as a string-key hash table. All
operations use an internal stack, so there is no recursion.

## API

See `trie.h` for complete API documentation.

~~~c
trie  *trie_create(void);
int    trie_free(trie *trie);
void  *trie_search(const trie *trie, const char *key);
int    trie_insert(trie *trie, const char *key, void *data);
int    trie_replace(trie *trie, const char *key, trie_replacer f, void *arg);
int    trie_visit(trie *trie, const char *prefix, trie_visitor v, void *arg);
int    trie_prune(trie *trie);
size_t trie_count(trie *trie, const char *prefix);
size_t trie_size(trie *trie);

/* Iterator */
trie_it    *trie_it_create(trie *, const char *prefix);
int         trie_it_next(trie_it *);
const char *trie_it_key(trie_it *);
void       *trie_it_data(trie_it *);
int         trie_it_done(trie_it *);
int         trie_it_error(trie_it *);
void        trie_it_free(trie_it *);
~~~

#ifndef NP_TRIE_H
#define NP_TRIE_H

#include <stddef.h>

typedef struct trie trie_t;

typedef int (*trie_visitor_t)(const char *key, void *data, void *arg);
typedef void *(*trie_replacer_t)(const char *key, void *current, void *arg);

/**
 * @return a freshly allocated trie, NULL on allocation error
 */
trie_t *
trie_create();

/**
 * Destroys a trie created by trie_create().
 */
void
trie_free(trie_t *trie);

/**
 * Finds for the data associated with KEY.
 * @return the previously inserted data
 */
void *
trie_search(const trie_t *trie, const char *key);

/**
 * Insert or replace DATA associated with KEY. Inserting NULL is the
 * equivalent of unassociating that key, though no memory will be
 * released.
 * @return 0 on success
 */
int
trie_insert(trie_t *trie, const char *key, void *data);

/**
 * Replace data associated with KEY using a replacer function. The
 * replacer function gets the key, the original data (NULL if none)
 * and ARG. Its return value is inserted into the trie.
 * @return 0 on success
 */
int
trie_replace(trie_t *trie, const char *key, trie_replacer_t f, void *arg);

/**
 * Visit in lexicographical order each key that matches the prefix. An
 * empty prefix visits every key in the trie. The visitor must accept
 * three arguments: the key, the data, and ARG. Iteration is aborted
 * (with success) if visitor returns non-zero.
 * @return 0 on success
 */
int
trie_visit(trie_t *trie, const char *prefix, trie_visitor_t visitor, void *arg);

/**
 * Count the number of entries with a given prefix. An empty prefix
 * counts the entire trie.
 * @return the number of entries matching PREFIX
 */
size_t
trie_count(trie_t *trie, const char *prefix);

/**
 * @return the number of bytes of memory used by this trie
 */
size_t
trie_size(trie_t *trie);

#endif

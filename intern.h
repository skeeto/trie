#ifndef NP_INTERN_H
#define NP_INTERN_H

struct intern {
    trie_t *trie;
};

/**
 * Initializes a new string intern string pool.
 * @return 0 on success
 */
int
intern_init(struct intern *pool);

/**
 * Frees all resources held by an intern string pool.
 */
void
intern_free(struct intern *pool);

/**
 * @return the canonical object for STRING, NULL on error
 */
const char *
intern(struct intern *pool, const char *string);

/**
 * @return the canonical object for STRING, NULL if none exists
 */
const char *
intern_soft(struct intern *pool, const char *string);

/**
 * @return the number of strings in this pool
 */
size_t
intern_count(struct intern *pool);

#endif

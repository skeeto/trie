#pragma once

/**
 * String Pool Library
 *
 * Interns and allocates internal copies of provided strings. The
 * intern() function returns the canonical string for a given UTF-8,
 * NUL-terminated C string. This provides two significant advantages:
 *
 * 1. Canonicalized strings compare correctly with ==. No strcmp() is
 *    required.
 * 2. Strings are only ever allocated once. For this to work properly,
 *    canonicalized strings MUST be considered immutable.
 *
 * The underlying trie is intentionally exposed for direct access,
 * should any sort of trie-specific functionality be required.
 */

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
 * @return 0 on success
 */
int
intern_free(struct intern *pool);

/**
 * Retrieves the canonical string, inserting one if necessary.
 * @return the canonical object for STRING, NULL on error
 */
const char *
intern(struct intern *pool, const char *string);

/**
 * Get the canonical string without adding a new string to the pool.
 * @return the canonical object for STRING, NULL if none exists
 */
const char *
intern_soft(struct intern *pool, const char *string);

/**
 * @return the number of strings in this pool
 */
size_t
intern_count(struct intern *pool);

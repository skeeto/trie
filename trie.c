#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <errno.h>
#include "trie.h"

/* Mark recursive functions since they're somewhat dangerous. They'll
 * fail when very long strings are inserted. TODO: Make these
 * non-recursive by building a heap stack. */
#define RECURSIVE

struct trieptr {
    trie_t *trie;
    int c;
};

struct trie {
    void *data;
    uint8_t nchildren, size;
    struct trieptr children[];
};

trie_t *trie_create()
{
    /* Root never needs to be resized. */
    size_t tail_size = sizeof(struct trieptr) * 255;
    trie_t *root = malloc(sizeof(*root) + tail_size);
    if (root == NULL)
        return NULL;
    root->size = 255;
    root->nchildren = 0;
    root->data = NULL;
    return root;
}

RECURSIVE void trie_free(trie_t *trie)
{
    for (int i = 0; i < trie->nchildren; i++)
        trie_free(trie->children[i].trie);
    free(trie);
}

static size_t
binary_search(trie_t *trie, trie_t **child,
              struct trieptr **ptr, const char *key)
{
    size_t i = 0;
    bool found = true;
    *ptr = NULL;
    while (found && key[i] != '\0') {
        int first = 0;
        int last = trie->nchildren - 1;
        int middle;
        found = false;
        while (first <= last) {
            middle = (first + last) / 2;
            struct trieptr *p = &trie->children[middle];
            if (p->c < key[i]) {
                first = middle + 1;
            } else if (p->c == key[i]) {
                trie = p->trie;
                *ptr = p;
                found = true;
                i++;
                break;
            } else {
                last = middle - 1;
            }
        }
    }
    *child = trie;
    return i;
}

void *trie_search(const trie_t *trie, const char *key)
{
    trie_t *child;
    struct trieptr *parent;
    size_t depth = binary_search((trie_t *) trie, &child, &parent, key);
    return key[depth] == '\0' ? child->data : NULL;
}

static trie_t *grow(trie_t *trie) {
    int size = trie->size * 2;
    if (size > 255)
        size = 255;
    size_t children_size = sizeof(struct trieptr) * size;
    trie_t *resized = realloc(trie, sizeof(*trie) + children_size);
    if (resized == NULL)
        return NULL;
    resized->size = size;
    return resized;

}

static int ptr_cmp(const void *a, const void *b)
{
    return ((struct trieptr *)a)->c - ((struct trieptr *)b)->c;
}

static trie_t *add(trie_t *trie, int c, trie_t *child)
{
    if (trie->size == trie->nchildren) {
        trie = grow(trie);
        if (trie == NULL)
            return NULL;
    }
    int i = trie->nchildren++;
    trie->children[i].c = c;
    trie->children[i].trie = child;
    qsort(trie->children, trie->nchildren, sizeof(trie->children[0]), ptr_cmp);
    return trie;
}

static trie_t *create()
{
    int size = 1;
    trie_t *trie = malloc(sizeof(*trie) + sizeof(struct trieptr) * size);
    if (trie == NULL)
        return NULL;
    trie->size = size;
    trie->nchildren = 0;
    trie->data = NULL;
    return trie;
}

static void *identity(const char *key, void *data, void *arg)
{
    (void) key;
    (void) data;
    return arg;
}

int trie_replace(trie_t *trie, const char *key, trie_replacer_t f, void *arg)
{
    trie_t *last;
    struct trieptr *parent;
    size_t depth = binary_search(trie, &last, &parent, key);
    while (key[depth] != '\0') {
        trie_t *subtrie = create();
        if (subtrie == NULL)
            return errno;
        trie_t *added = add(last, key[depth], subtrie);
        if (added == NULL) {
            free(subtrie);
            return errno;
        }
        if (parent != NULL) {
            parent->trie = added;
            parent = NULL;
        }
        last = subtrie;
        depth++;
    }
    last->data = f(key, last->data, arg);
    return 0;
}

int trie_insert(trie_t *trie, const char *key, void *data)
{
    return trie_replace(trie, key, identity, data);
}

RECURSIVE static int
visit(trie_t *trie, char *key, size_t *keysize, size_t depth,
      trie_visitor_t visitor, void *arg)
{
    if (*keysize == depth - 1) {
        *keysize *= 2;
        char *nextkey = realloc(key, *keysize);
        if (nextkey == NULL)
            return -1;
        key = nextkey;
    }
    if (trie->data)
        if (visitor(key, trie->data, arg) != 0)
            return 1;
    for (int i = 0; i < trie->nchildren; i++) {
        key[depth] = trie->children[i].c;
        trie_t *child = trie->children[i].trie;
        int r = visit(child, key, keysize, depth + 1, visitor, arg);
        if (r != 0)
            return r;
    }
    key[depth] = '\0';
    return 0;
}

int
trie_visit(trie_t *trie, const char *prefix, trie_visitor_t visitor, void *arg)
{
    trie_t *start = trie;
    struct trieptr *ptr;
    int depth = binary_search(trie, &start, &ptr, prefix);
    if (prefix[depth] != '\0')
        return 0;
    size_t keysize = strlen(prefix) * 4 + 1;
    char *key = calloc(keysize, 1);
    if (key == NULL)
        return errno;
    strcpy(key, prefix);
    int r = visit(start, key, &keysize, depth, visitor, arg);
    free(key);
    return r >= 0 ? 0 : -1;
}

static int visitor_counter(const char *key, void *data, void *arg)
{
    size_t *count = arg;
    count[0]++;
    return 0;
}

size_t trie_count(trie_t *trie, const char *prefix)
{
    size_t count = 0;
    trie_visit(trie, prefix, visitor_counter, &count);
    return count;
}

RECURSIVE size_t trie_size(trie_t *trie)
{
    size_t size = sizeof(*trie) + sizeof(*trie->children) * trie->size;
    for (int i = 0; i < trie->nchildren; i++) {
        size += trie_size(trie->children[i].trie);
    }
    return size;
}

#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <errno.h>
#include "trie.h"

struct trieptr {
    struct trie *trie;
    int c;
};

struct trie {
    void *data;
    uint8_t nchildren, size;
    struct trieptr children[];
};

struct trie *trie_create()
{
    size_t tail_size = sizeof(struct trieptr) * 255;
    struct trie *root = malloc(sizeof(*root) + tail_size);
    if (root == NULL)
        return NULL;
    root->size = 255;
    root->nchildren = 0;
    root->data = NULL;
    return root;
}

static int
binary_search(struct trie *trie, struct trie **child,
              struct trieptr **ptr, const char *string)
{
    int i = 0;
    bool found = true;
    *ptr = NULL;
    while (found && string[i] != '\0') {
        int first = 0;
        int last = trie->nchildren - 1;
        int middle;
        found = false;
        while (first <= last) {
            middle = (first + last) / 2;
            struct trieptr *p = &trie->children[middle];
            if (p->c < string[i]) {
                first = middle + 1;
            } else if (p->c == string[i]) {
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

void *trie_search(const struct trie *trie, const char *string)
{
    struct trie *child;
    struct trieptr *parent;
    int depth = binary_search((struct trie *) trie, &child, &parent, string);
    return string[depth] == '\0' ? child->data : NULL;
}

static struct trie *grow(struct trie *trie) {
    int size = trie->size * 2;
    if (size > 255)
        size = 255;
    size_t children_size = sizeof(struct trieptr) * size;
    struct trie *resized = realloc(trie, sizeof(*trie) + children_size);
    if (resized == NULL)
        return NULL;
    resized->size = size;
    return resized;

}

static int ptr_cmp(const void *a, const void *b)
{
    return ((struct trieptr *)a)->c - ((struct trieptr *)b)->c;
}

static struct trie *
add(struct trie *trie, int c, struct trie *child)
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

static struct trie *create()
{
    int size = 1;
    struct trie *trie = malloc(sizeof(*trie) + sizeof(struct trieptr) * size);
    if (trie == NULL)
        return NULL;
    trie->size = size;
    trie->nchildren = 0;
    trie->data = NULL;
    return trie;
}

int trie_insert(struct trie *trie, const char *string, void *data)
{
    struct trie *last;
    struct trieptr *parent;
    int depth = binary_search(trie, &last, &parent, string);
    while (string[depth] != '\0') {
        struct trie *subtrie = create();
        if (subtrie == NULL)
            return errno;
        struct trie *added = add(last, string[depth], subtrie);
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
    last->data = data;
    return 0;
}

size_t trie_count(struct trie *trie)
{
    size_t count = trie->data != NULL;
    for (uint8_t i = 0; i < trie->nchildren; i++) {
        count += trie_count(trie->children[i].trie);
    }
    return count;
}

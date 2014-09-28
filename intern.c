#include <stdlib.h>
#include <string.h>
#include "trie.h"
#include "intern.h"

int intern_init(struct intern *pool)
{
    pool->trie = trie_create();
    return pool->trie == NULL ? -1 : 0;
}


static int visitor_free(const char *key, void *data, void *arg)
{
    (void) key;
    (void) arg;
    free(data);
    return 0;
}

void intern_free(struct intern *pool)
{
    trie_visit(pool->trie, "", visitor_free, NULL);
    trie_free(pool->trie);
}

const char *intern(struct intern *pool, const char *string)
{
    const char *data = trie_search(pool->trie, string);
    if (data != NULL) {
        return data;
    } else {
        char *dup = malloc(strlen(string) + 1);
        if (dup == NULL)
            return NULL;
        strcpy(dup, string);
        int r = trie_insert(pool->trie, string, dup);
        if (r != 0) {
            free(dup);
            return NULL;
        } else {
            return dup;
        }
    }
}

const char *intern_soft(struct intern *pool, const char *string)
{
    return trie_search(pool->trie, string);
}

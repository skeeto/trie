#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "trie.h"
#include "intern.h"

int visitor_print(const char *key, void *data, void *arg)
{
    printf("%s\n", key);
    return 0;
}

int main(int argc, char **argv)
{
    struct intern pool;
    if (intern_init(&pool) != 0) {
        fprintf(stderr, "error: intern pool failure\n");
        return -1;
    }
    char line[128];
    while (fgets(line, sizeof(line), stdin)) {
        line[strlen(line) - 1] = '\0';
        if (intern(&pool, line) == NULL) {
            printf("error: %s\n", line);
            return 1;
        }
    }
    printf("%zu words\n", trie_count(pool.trie, ""));
    printf("'%s'\n", (char *) trie_search(pool.trie, argv[1]));
    trie_visit(pool.trie, "xylot", visitor_print, NULL);
    intern_free(&pool);
    return 0;
}

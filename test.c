#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "trie.h"
#include "intern.h"

int visitor_print(const char *key, void *data, void *arg)
{
    (void) data;
    (void) arg;
    printf("%s\n", key);
    return 0;
}

int main(int argc, char **argv)
{
    (void) argc;
    struct intern pool;
    if (intern_init(&pool) != 0) {
        fprintf(stderr, "error: intern pool failure\n");
        return -1;
    }
    char line[128];
    while (fgets(line, sizeof(line), stdin)) {
        line[strlen(line) - 1] = '\0';
        if (intern(&pool, line) == NULL) {
            printf("error: could not insert '%s'\n", line);
            return 1;
        }
    }
    printf("%zu words\n", intern_count(&pool));
    printf("%0.2f MB\n", trie_size(pool.trie) / 1024.0 / 1024.0);
    printf("'%s'\n", (char *) intern_soft(&pool, argv[1]));
    if (trie_visit(pool.trie, "xylot", visitor_print, NULL) != 0) {
        printf("error: visit failure\n");
        return 1;
    }
    if (intern_free(&pool) != 0) {
        printf("error: free failure\n");
        return 1;
    }
    return 0;
}

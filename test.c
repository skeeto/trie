#define _BSD_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "trie.h"

/* void print(struct trie *trie, int indent) */
/* { */
/*     char space[indent * 2 + 1]; */
/*     memset(space, ' ', sizeof(space) - 1); */
/*     space[sizeof(space) - 1] = 0; */
/*     printf("%sTRIE (%d):\n", space, trie->size); */
/*     for (int i = 0; i < trie->nchildren; i++) { */
/*         printf("%s  %c:\n", space, trie->children[i].c); */
/*         print(trie->children[i].trie, indent + 1); */
/*     } */
/* } */

int visitor_print(const char *key, void *data, void *arg)
{
    printf("%s\n", key);
    return 0;
}

int visitor_free(const char *key, void *data, void *arg)
{
    free(data);
    return 0;
}

int main(int argc, char **argv)
{
    struct trie *trie = trie_create();
    char line[128];
    while (fgets(line, sizeof(line), stdin)) {
        line[strlen(line) - 1] = '\0';
        if (trie_insert(trie, line, strdup(line)) != 0) {
            printf("error: %s\n", line);
            return 1;
        }
    }
    printf("%zu words\n", trie_count(trie, ""));
    char *find = trie_search(trie, argv[1]);
    printf("'%s'\n", find);
    trie_visit(trie, "xylot", visitor_print, NULL);
    trie_visit(trie, "", visitor_free, NULL);
    trie_free(trie);
    return 0;
}

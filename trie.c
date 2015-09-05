#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <errno.h>
#include "trie.h"

struct trieptr {
    trie *trie;
    int c;
};

struct trie {
    void *data;
    uint8_t nchildren, size;
    struct trieptr children[];
};

/* Mini stack library for non-recursive traversal. */

struct stack_node {
    trie *trie;
    uint8_t i;
};

struct stack {
    struct stack_node *stack;
    size_t fill, size;
};

static inline int
stack_init(struct stack *s)
{
    s->size = 256;
    s->fill = 0;
    s->stack = malloc(s->size * sizeof(struct stack_node));
    return s->stack == NULL ? -1 : 0;
}

static inline void
stack_free(struct stack *s)
{
    free(s->stack);
    s->stack = NULL;
}

static inline int
stack_grow(struct stack *s)
{
    size_t newsize = s->size * 2 * sizeof(struct stack_node);
    struct stack_node *resize = realloc(s->stack, newsize);
    if (resize == NULL) {
        stack_free(s);
        return -1;
    }
    s->size *= 2;
    s->stack = resize;
    return 0;
}

static inline int
stack_push(struct stack *s, trie *trie)
{
    if (s->fill == s->size)
        if (stack_grow(s) != 0)
            return -1;
    s->stack[s->fill++] = (struct stack_node){trie, 0};
    return 0;
}

static inline trie *
stack_pop(struct stack *s)
{
    return s->stack[--s->fill].trie;
}

static inline struct stack_node *
stack_peek(struct stack *s)
{
    return &s->stack[s->fill - 1];
}

/* Constructor and destructor. */

trie *
trie_create(void)
{
    /* Root never needs to be resized. */
    size_t tail_size = sizeof(struct trieptr) * 255;
    trie *root = malloc(sizeof(*root) + tail_size);
    if (root == NULL)
        return NULL;
    root->size = 255;
    root->nchildren = 0;
    root->data = NULL;
    return root;
}

int
trie_free(trie *trie)
{
    struct stack stack, *s = &stack;
    if (stack_init(s) != 0)
        return errno;
    stack_push(s, trie); // first push always successful
    while (s->fill > 0) {
        struct stack_node *node = stack_peek(s);
        if (node->i < node->trie->nchildren) {
            if (stack_push(s, node->trie->children[node->i].trie) != 0)
                return errno;
            node->i++;
        } else {
            free(stack_pop(s));
        }
    }
    stack_free(s);
    return 0;
}

/* Core search functions. */

static size_t
binary_search(trie *self, trie **child,
              struct trieptr **ptr, const char *key)
{
    size_t i = 0;
    bool found = true;
    *ptr = NULL;
    while (found && key[i] != '\0') {
        int first = 0;
        int last = self->nchildren - 1;
        int middle;
        found = false;
        while (first <= last) {
            middle = (first + last) / 2;
            struct trieptr *p = &self->children[middle];
            if (p->c < key[i]) {
                first = middle + 1;
            } else if (p->c == key[i]) {
                self = p->trie;
                *ptr = p;
                found = true;
                i++;
                break;
            } else {
                last = middle - 1;
            }
        }
    }
    *child = self;
    return i;
}

void *
trie_search(const trie *self, const char *key)
{
    trie *child;
    struct trieptr *parent;
    size_t depth = binary_search((trie *) self, &child, &parent, key);
    return key[depth] == '\0' ? child->data : NULL;
}

/* Insertion functions. */

static trie *
grow(trie *self) {
    int size = self->size * 2;
    if (size > 255)
        size = 255;
    size_t children_size = sizeof(struct trieptr) * size;
    trie *resized = realloc(self, sizeof(*self) + children_size);
    if (resized == NULL)
        return NULL;
    resized->size = size;
    return resized;
}

static int
ptr_cmp(const void *a, const void *b)
{
    return ((struct trieptr *)a)->c - ((struct trieptr *)b)->c;
}

static trie *
add(trie *self, int c, trie *child)
{
    if (self->size == self->nchildren) {
        self = grow(self);
        if (self == NULL)
            return NULL;
    }
    int i = self->nchildren++;
    self->children[i].c = c;
    self->children[i].trie = child;
    qsort(self->children, self->nchildren, sizeof(self->children[0]), ptr_cmp);
    return self;
}

static trie *
create(void)
{
    int size = 1;
    trie *trie = malloc(sizeof(*trie) + sizeof(struct trieptr) * size);
    if (trie == NULL)
        return NULL;
    trie->size = size;
    trie->nchildren = 0;
    trie->data = NULL;
    return trie;
}

static void *
identity(const char *key, void *data, void *arg)
{
    (void) key;
    (void) data;
    return arg;
}

int
trie_replace(trie *self, const char *key, trie_replacer f, void *arg)
{
    trie *last;
    struct trieptr *parent;
    size_t depth = binary_search(self, &last, &parent, key);
    while (key[depth] != '\0') {
        trie *subtrie = create();
        if (subtrie == NULL)
            return errno;
        trie *added = add(last, key[depth], subtrie);
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

int
trie_insert(trie *trie, const char *key, void *data)
{
    return trie_replace(trie, key, identity, data);
}

/* Mini buffer library. */

struct buffer {
    char *buffer;
    size_t size, fill;
};

static inline int
buffer_init(struct buffer *b, const char *prefix)
{
    b->fill = strlen(prefix);
    b->size = b->fill > 256 ? b->fill * 2 : 256;
    b->buffer = malloc(b->size);
    if (b->buffer != NULL)
        strcpy(b->buffer, prefix);
    return b->buffer == NULL ? -1 : 0;
}

static inline void
buffer_free(struct buffer *b)
{
    free(b->buffer);
    b->buffer = NULL;
}

static inline int
buffer_grow(struct buffer *b)
{
    char *resize = realloc(b->buffer, b->size * 2);
    if (resize == NULL) {
        buffer_free(b);
        return -1;
    }
    b->buffer = resize;
    b->size *= 2;
    return 0;
}

static inline int
buffer_push(struct buffer *b, char c)
{
    if (b->fill + 1 == b->size)
        if (buffer_grow(b) != 0)
            return -1;
    b->buffer[b->fill++] = c;
    b->buffer[b->fill] = '\0';
    return 0;
}

static inline void
buffer_pop(struct buffer *b)
{
    if (b->fill > 0)
        b->buffer[--b->fill] = '\0';
}

/* Core visitation functions. */

static int
visit(trie *self, const char *prefix, trie_visitor visitor, void *arg)
{
    struct buffer buffer, *b = &buffer;
    struct stack stack, *s = &stack;
    if (buffer_init(b, prefix) != 0)
        return -1;
    if (stack_init(s) != 0) {
        buffer_free(b);
        return -1;
    }
    stack_push(s, self);
    while (s->fill > 0) {
        struct stack_node *node = stack_peek(s);
        if (node->i == 0 && node->trie->data != NULL) {
            if (visitor(b->buffer, node->trie->data, arg) != 0) {
                buffer_free(b);
                stack_free(s);
                return 1;
            }
        }
        if (node->i < node->trie->nchildren) {
            if (stack_push(s, node->trie->children[node->i].trie) != 0) {
                buffer_free(b);
                return -1;
            }
            if (buffer_push(b, node->trie->children[node->i].c) != 0) {
                stack_free(s);
                return -1;
            }
            node->i++;
        } else {
            buffer_pop(b);
            stack_pop(s);
        }
    }
    buffer_free(b);
    stack_free(s);
    return 0;
}

int
trie_visit(trie *self, const char *prefix, trie_visitor visitor, void *arg)
{
    trie *start = self;
    struct trieptr *ptr;
    int depth = binary_search(self, &start, &ptr, prefix);
    if (prefix[depth] != '\0')
        return 0;
    int r = visit(start, prefix, visitor, arg);
    return r >= 0 ? 0 : -1;
}

/* Miscellaneous functions. */

static int
visitor_counter(const char *key, void *data, void *arg)
{
    (void) key;
    (void) data;
    size_t *count = arg;
    count[0]++;
    return 0;
}

size_t
trie_count(trie *trie, const char *prefix)
{
    size_t count = 0;
    trie_visit(trie, prefix, visitor_counter, &count);
    return count;
}

size_t
trie_size(trie *trie)
{
    struct stack stack, *s = &stack;
    if (stack_init(s) != 0)
        return 0;
    stack_push(s, trie);
    size_t size = 0;
    while (s->fill > 0) {
        struct stack_node *node = stack_peek(s);
        if (node->i < node->trie->nchildren) {
            if (stack_push(s, node->trie->children[node->i].trie) != 0)
                return 0;
            node->i++;
        } else {
            struct trie *t = stack_pop(s);
            size += sizeof(*t) + sizeof(*t->children) * t->size;
        }
    }
    stack_free(s);
    return size;
}

#include <stdlib.h>
#include <string.h>
#include "trie.h"

struct trieptr {
    struct trie *trie;
    int c;
};

struct trie {
    void *data;
    short nchildren, size;
    struct trieptr children[];
};

/* Mini stack library for non-recursive traversal. */

struct stack_node {
    struct trie *trie;
    int i;
};

struct stack {
    struct stack_node *stack;
    size_t fill, size;
};

static int
stack_init(struct stack *s)
{
    s->size = 256;
    s->fill = 0;
    s->stack = malloc(s->size * sizeof(struct stack_node));
    return !s->stack ? -1 : 0;
}

static void
stack_free(struct stack *s)
{
    free(s->stack);
    s->stack = 0;
}

static int
stack_grow(struct stack *s)
{
    size_t newsize = s->size * 2 * sizeof(struct stack_node);
    struct stack_node *resize = realloc(s->stack, newsize);
    if (!resize) {
        stack_free(s);
        return -1;
    }
    s->size *= 2;
    s->stack = resize;
    return 0;
}

static int
stack_push(struct stack *s, struct trie *trie)
{
    if (s->fill == s->size)
        if (stack_grow(s) != 0)
            return -1;
    s->stack[s->fill++] = (struct stack_node){trie, 0};
    return 0;
}

static struct trie *
stack_pop(struct stack *s)
{
    return s->stack[--s->fill].trie;
}

static struct stack_node *
stack_peek(struct stack *s)
{
    return &s->stack[s->fill - 1];
}

/* Constructor and destructor. */

struct trie *
trie_create(void)
{
    /* Root never needs to be resized. */
    size_t tail_size = sizeof(struct trieptr) * 255;
    struct trie *root = malloc(sizeof(*root) + tail_size);
    if (!root)
        return 0;
    root->size = 255;
    root->nchildren = 0;
    root->data = 0;
    return root;
}

int
trie_free(struct trie *trie)
{
    struct stack stack, *s = &stack;
    if (stack_init(s) != 0)
        return 1;
    stack_push(s, trie); /* first push always successful */
    while (s->fill > 0) {
        struct stack_node *node = stack_peek(s);
        if (node->i < node->trie->nchildren) {
            int i = node->i++;
            if (stack_push(s, node->trie->children[i].trie) != 0)
                return 1;
        } else {
            free(stack_pop(s));
        }
    }
    stack_free(s);
    return 0;
}

/* Core search functions. */

static size_t
binary_search(struct trie *self, struct trie **child,
              struct trieptr **ptr, const unsigned char *key)
{
    size_t i = 0;
    int found = 1;
    *ptr = 0;
    while (found && key[i]) {
        int first = 0;
        int last = self->nchildren - 1;
        int middle;
        found = 0;
        while (first <= last) {
            middle = (first + last) / 2;
            struct trieptr *p = &self->children[middle];
            if (p->c < key[i]) {
                first = middle + 1;
            } else if (p->c == key[i]) {
                self = p->trie;
                *ptr = p;
                found = 1;
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
trie_search(const struct trie *self, const char *key)
{
    struct trie *child;
    struct trieptr *parent;
    unsigned char *ukey = (unsigned char *)key;
    size_t depth = binary_search((struct trie *)self, &child, &parent, ukey);
    return !key[depth] ? child->data : 0;
}

/* Insertion functions. */

static struct trie *
grow(struct trie *self) {
    int size = self->size * 2;
    if (size > 255)
        size = 255;
    size_t children_size = sizeof(struct trieptr) * size;
    struct trie *resized = realloc(self, sizeof(*self) + children_size);
    if (!resized)
        return 0;
    resized->size = size;
    return resized;
}

static int
ptr_cmp(const void *a, const void *b)
{
    return ((struct trieptr *)a)->c - ((struct trieptr *)b)->c;
}

static struct trie *
node_add(struct trie *self, int c, struct trie *child)
{
    if (self->size == self->nchildren) {
        self = grow(self);
        if (!self)
            return 0;
    }
    int i = self->nchildren++;
    self->children[i].c = c;
    self->children[i].trie = child;
    qsort(self->children, self->nchildren, sizeof(self->children[0]), ptr_cmp);
    return self;
}

static void
node_remove(struct trie *self, int i)
{
    size_t len = (--self->nchildren - i) * sizeof(self->children[0]);
    memmove(self->children + i, self->children + i + 1, len);
}

static struct trie *
create(void)
{
    int size = 1;
    struct trie *trie = malloc(sizeof(*trie) + sizeof(struct trieptr) * size);
    if (!trie)
        return 0;
    trie->size = size;
    trie->nchildren = 0;
    trie->data = 0;
    return trie;
}

static void *
identity(const char *key, void *data, void *arg)
{
    (void)key;
    (void)data;
    return arg;
}

int
trie_replace(struct trie *self, const char *key, trie_replacer f, void *arg)
{
    struct trie *last;
    struct trieptr *parent;
    unsigned char *ukey = (unsigned char *)key;
    size_t depth = binary_search(self, &last, &parent, ukey);
    while (ukey[depth]) {
        struct trie *subtrie = create();
        if (!subtrie)
            return 1;
        struct trie *added = node_add(last, ukey[depth], subtrie);
        if (!added) {
            free(subtrie);
            return 1;
        }
        if (parent) {
            parent->trie = added;
            parent = 0;
        }
        last = subtrie;
        depth++;
    }
    last->data = f(key, last->data, arg);
    return 0;
}

int
trie_insert(struct trie *trie, const char *key, void *data)
{
    return trie_replace(trie, key, identity, data);
}

/* Mini buffer library. */

struct buffer {
    char *buffer;
    size_t size, fill;
};

static int
buffer_init(struct buffer *b, const char *prefix)
{
    b->fill = strlen(prefix);
    b->size = b->fill >= 256 ? b->fill * 2 : 256;
    b->buffer = malloc(b->size);
    if (b->buffer)
        memcpy(b->buffer, prefix, b->fill + 1);
    return !b->buffer ? -1 : 0;
}

static void
buffer_free(struct buffer *b)
{
    free(b->buffer);
    b->buffer = 0;
}

static int
buffer_grow(struct buffer *b)
{
    char *resize = realloc(b->buffer, b->size * 2);
    if (!resize) {
        buffer_free(b);
        return -1;
    }
    b->buffer = resize;
    b->size *= 2;
    return 0;
}

static int
buffer_push(struct buffer *b, char c)
{
    if (b->fill + 1 == b->size)
        if (buffer_grow(b) != 0)
            return -1;
    b->buffer[b->fill++] = c;
    b->buffer[b->fill] = 0;
    return 0;
}

static void
buffer_pop(struct buffer *b)
{
    if (b->fill > 0)
        b->buffer[--b->fill] = 0;
}

/* Core visitation functions. */

static int
visit(struct trie *self, const char *prefix, trie_visitor visitor, void *arg)
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
        if (node->i == 0 && node->trie->data) {
            if (visitor(b->buffer, node->trie->data, arg) != 0) {
                buffer_free(b);
                stack_free(s);
                return 1;
            }
        }
        if (node->i < node->trie->nchildren) {
            struct trie *trie = node->trie->children[node->i].trie;
            int c = node->trie->children[node->i].c;
            node->i++;
            if (stack_push(s, trie) != 0) {
                buffer_free(b);
                return -1;
            }
            if (buffer_push(b, c) != 0) {
                stack_free(s);
                return -1;
            }
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
trie_visit(struct trie *self, const char *prefix, trie_visitor v, void *arg)
{
    struct trie *start = self;
    struct trieptr *ptr;
    unsigned char *uprefix = (unsigned char *)prefix;
    int depth = binary_search(self, &start, &ptr, uprefix);
    if (prefix[depth])
        return 0;
    int r = visit(start, prefix, v, arg);
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
trie_count(struct trie *trie, const char *prefix)
{
    size_t count = 0;
    trie_visit(trie, prefix, visitor_counter, &count);
    return count;
}

int
trie_prune(struct trie *trie)
{
    struct stack stack, *s = &stack;
    if (stack_init(s) != 0)
        return -1;
    stack_push(s, trie);
    while (s->fill > 0) {
        struct stack_node *node = stack_peek(s);
        int i = node->i++;
        if (i < node->trie->nchildren) {
            if (stack_push(s, node->trie->children[i].trie) != 0)
                return 0;
        } else {
            struct trie *t = stack_pop(s);
            for (int i = 0; i < t->nchildren; i++) {
                struct trie *child = t->children[i].trie;
                if (!child->nchildren && !child->data) {
                    node_remove(t, i--);
                    free(child);
                }
            }
        }
    }
    stack_free(s);
    return 1;
}

size_t
trie_size(struct trie *trie)
{
    struct stack stack, *s = &stack;
    if (stack_init(s) != 0)
        return 0;
    stack_push(s, trie);
    size_t size = 0;
    while (s->fill > 0) {
        struct stack_node *node = stack_peek(s);
        int i = node->i++;
        if (i < node->trie->nchildren) {
            if (stack_push(s, node->trie->children[i].trie) != 0)
                return 0;
        } else {
            struct trie *t = stack_pop(s);
            size += sizeof(*t) + sizeof(*t->children) * t->size;
        }
    }
    stack_free(s);
    return size;
}

struct trie_it {
    struct stack stack;
    struct buffer buffer;
    void *data;
    int error;
};

struct trie_it *
trie_it_create(struct trie *trie, const char *prefix)
{
    struct trie_it *it = malloc(sizeof(*it));
    if (!it)
        return 0;
    if (stack_init(&it->stack)) {
        free(it);
        return 0;
    }
    if (buffer_init(&it->buffer, prefix)) {
        stack_free(&it->stack);
        free(it);
        return 0;
    }
    stack_push(&it->stack, trie); /* first push always successful */
    it->data = 0;
    it->error = 0;
    trie_it_next(it);
    return it;
}

int
trie_it_next(struct trie_it *it)
{
    while (!it->error && it->stack.fill) {
        struct stack_node *node = stack_peek(&it->stack);

        if (node->i == 0 && node->trie->data) {
            if (!it->data) {
                it->data = node->trie->data;
                return 1;
            } else {
                it->data = 0;
            }
        }

        if (node->i < node->trie->nchildren) {
            struct trie *trie = node->trie->children[node->i].trie;
            int c = node->trie->children[node->i].c;
            node->i++;
            if (stack_push(&it->stack, trie)) {
                it->error = 1;
                return 0;
            }
            if (buffer_push(&it->buffer, c)) {
                it->error = 1;
                return 0;
            }
        } else {
            buffer_pop(&it->buffer);
            stack_pop(&it->stack);
        }
    }
    return 0;
}

const char *
trie_it_key(struct trie_it *it)
{
    return it->buffer.buffer;
}

void *
trie_it_data(struct trie_it *it)
{
    return it->data;
}

int
trie_it_done(struct trie_it *it)
{
    return it->error || !it->stack.fill;
}

int
trie_it_error(struct trie_it *it)
{
    return it->error;
}

void
trie_it_free(struct trie_it *it)
{
    buffer_free(&it->buffer);
    stack_free(&it->stack);
    free(it);
}

#ifndef AHB_LIB
#define AHB_LIB

#include <stdlib.h>
#include <assert.h>

typedef struct {
    int curr_argn;
    int total_args;
    char **args;
} State;

typedef struct {
    char *data;
    size_t capacity;
    size_t count;
} TempAlloc;
static TempAlloc allocator = {0};

// cli arguments management
void ahb_args_init(State *st, int argc, char **argv);
char *ahb_args_next(State *st);
char *ahb_temp_alloc(const char *content);
void ahb_temp_alloc_free();
void __make_sure_is_initialized();

#endif  //AHB_LIB

// strip ahb prefix when non conflict names is assured
#ifdef AHB_STRIP_PREFIX

#define args_init ahb_args_init
#define args_next ahb_args_next
#define temp_alloc ahb_temp_alloc
#define temp_alloc_free ahb_temp_alloc_free

#endif // AHB_STRIP_PREFIX


#ifdef AHB_LIB_IMPLEMENTATION

void ahb_args_init(State *st, int argc, char **argv)
{
    st->curr_argn = 0;
    st->total_args = argc;
    st->args = argv;
}

char *ahb_args_next(State *st)
{
    st->curr_argn += 1;
    if (st->curr_argn >= st->total_args) return NULL;
    char *res = st->args[st->curr_argn];
    return res;
}

void _make_sure_is_initialized()
{
    if (allocator.data == NULL) {
        allocator.capacity = 1024 * 1024; // 1 KB
        allocator.data = malloc(allocator.capacity);
        assert(allocator.data);
    }
}

char *ahb_temp_alloc(const char *content)
{
    if (content == NULL) return NULL;
    _make_sure_is_initialized();
    size_t size = strlen(content);
    if (allocator.count > allocator.capacity) {
        allocator.capacity *= 2;
        allocator.data = realloc(allocator.data, allocator.capacity);
        assert(allocator.data);
    }
    memcpy(allocator.data + allocator.count, content, size);
    allocator.data[allocator.count + size] = '\0';
    char *ptr = allocator.data + allocator.count;
    allocator.count += size;
    return ptr;
}

void ahb_temp_alloc_free()
{
    free(allocator.data);
    allocator.capacity = 0;
    allocator.count = 0;
    allocator.data = NULL;
}

#endif // AHB_LIB_IMPLEMENTATION


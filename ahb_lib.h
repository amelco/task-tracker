#ifndef AHB_LIB
#define AHB_LIB

#include <stdlib.h>
#include <assert.h>
#include <stdbool.h>
#include <string.h>

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

#define AHB_TODO(msg)                            \
  printf("%s:%d: TODO (", __FILE__, __LINE__); \
  printf(msg")\n");                                   \
  exit(1)

#define da_append(p, item) \
    do {\
    if (!p->data || p->count >= p->capacity) { \
        p->capacity = !p->data ? 1 : p->capacity * 2; \
        p->data = realloc(p->data, p->capacity * sizeof((item))); \
    } \
    p->data[p->count++] = (item); \
    } while(0);

// cli arguments management
void ahb_args_init(State *st, int argc, char **argv);
char *ahb_args_next(State *st);
char *ahb_temp_alloc(const char *content);
void ahb_temp_alloc_free();
long ahb_get_file_size(FILE *f);

// file operations
char *ahb_read_entire_file(const char *path);

// miscelaneous
bool ahb_is_valid_date(const char *date);

#endif  //AHB_LIB

// strip ahb prefix when non conflict names is assured
#ifdef AHB_STRIP_PREFIX

#define args_init ahb_args_init
#define args_next ahb_args_next
#define temp_alloc ahb_temp_alloc
#define temp_alloc_free ahb_temp_alloc_free
#define read_entire_file ahb_read_entire_file
#define is_valid_date ahb_is_valid_date
#define get_file_size ahb_get_file_size
#define TODO AHB_TODO

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
        allocator.count = 0;
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
    allocator.count += size + 1;
    return ptr;
}

void ahb_temp_alloc_free()
{
    free(allocator.data);
    allocator.capacity = 0;
    allocator.count = 0;
    allocator.data = NULL;
}

long ahb_get_file_size(FILE *f)
{
    assert(f && "Could not open file");
    fseek(f, 0L, SEEK_END);
    size_t size = ftell(f);
    rewind(f);
    return size;
}

// this allocates memory
char *ahb_read_entire_file(const char *path)
{
    FILE *f = fopen(path, "r");
    size_t size = ahb_get_file_size(f);
    char *content = malloc(size + 1);
    assert(content && "Could not allocate memory. Buy more RAM.");
    int count = fread(content, 1, size, f);
    content[size] = '\0';
    assert((size_t)count == size && "Could not read entire file");
    fclose(f);
    return content;
}

bool ahb_is_valid_date(const char *date)
{
    // accepts only DD-MM-YYYY
    (void)date;
    AHB_TODO("implement is valid date");
    return true;
}

#endif // AHB_LIB_IMPLEMENTATION


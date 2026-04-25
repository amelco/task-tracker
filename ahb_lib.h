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

typedef struct {
    char **data;
    size_t capacity;
    size_t count;
} StringBuilder;

/* ---- Macros -------------------------------------------------------------- */
#define AHB_TODO(msg)                                \
    do {                                             \
        printf("%s:%d: TODO (", __FILE__, __LINE__); \
        printf(msg")\n");                            \
        exit(1);                                     \
    } while (0)

#define AHB_UNREACHABLE(msg)                                \
    do {                                             \
        printf("%s:%d: UNREACHABLE (", __FILE__, __LINE__); \
        printf(msg")\n");                            \
        exit(1);                                     \
    } while (0)

#define da_append(p, item)                                                  \
    do {                                                                    \
        if (!(p)->data || (p)->count >= (p)->capacity) {                    \
            (p)->capacity = !(p)->data ? 1 : (p)->capacity * 2;             \
            (p)->data = realloc((p)->data, (p)->capacity * sizeof((item))); \
        }                                                                   \
        (p)->data[(p)->count++] = (item);                                   \
    } while(0)
 
#define da_free(p) \
    free(p->data)
/* -------------------------------------------------------------------------- */

// command line arguments management
void ahb_args_init(State *st, int argc, char **argv);
char *ahb_args_next(State *st);
char *ahb_temp_alloc(const char *content);
void ahb_temp_alloc_free();

// file operations
long ahb_get_file_size(FILE *f);
char *ahb_read_entire_file(const char *path);

// String operations
void ahb_sb_append(StringBuilder *dest, const char *text); 

// utility functions
bool ahb_is_valid_date(const char *date);

#endif  //AHB_LIB

// strip ahb prefix when non conflict names is assured
#ifdef AHB_STRIP_PREFIX
#define args_init          ahb_args_init
#define args_next          ahb_args_next
#define temp_alloc         ahb_temp_alloc
#define temp_alloc_free    ahb_temp_alloc_free
#define read_entire_file   ahb_read_entire_file
#define is_valid_date      ahb_is_valid_date
#define get_file_size      ahb_get_file_size
#define sb_append          ahb_sb_append
#define sb_to_cstring      ahb_sb_to_cstring
#define TODO               AHB_TODO
#define UNREACHABLE        AHB_UNREACHABLE
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

// this funciont allocates memory if sb is empty
void ahb_sb_append(StringBuilder *sb, const char *text)
{
    da_append(sb, (char *)text);
}

// this function allocates memory and frees the string builder
char *ahb_sb_to_cstring(StringBuilder *sb)
{
    size_t allocSize = 0;
    for (size_t i = 0; i < sb->count; i++) {
        allocSize += strlen(sb->data[i]);
    }
    char *result = malloc(allocSize);
    
    size_t pos = 0;
    for (size_t i=0; i < sb->count; i++) {
        char *data = sb->data[i];
        strcpy(result + pos, data);
        pos += strlen(data);
    }
    da_free(sb);
    return result;
}

#endif // AHB_LIB_IMPLEMENTATION


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
    char *data;
    size_t capacity;
    size_t count;
} StringBuilder;

typedef struct {
    char **data;
    size_t capacity;
    size_t count;
} CStr_List;

typedef struct {
    int day;
    int month;
    int year;
} Date;

/* ---- Macros -------------------------------------------------------------- */
#define AHB_TODO(msg)                                \
    do {                                             \
        printf("%s:%d: TODO (", __FILE__, __LINE__); \
        printf(msg")\n");                            \
        exit(1);                                     \
    } while (0)

#define ABORT(msg) \
    do { \
        printf("ERROR: %s\n", msg); \
        exit(1); \
    } while(0)

#define AHB_UNREACHABLE(msg)                                \
    do {                                                    \
        printf("%s:%d: UNREACHABLE (", __FILE__, __LINE__); \
        printf(msg")\n");                                   \
        exit(1);                                            \
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
void ahb_split(const char *cstr, char sep, CStr_List *l);

// utility functions
bool ahb_is_valid_date(const char *date, long long *date_lld);
long long date_to_lld(const char *date);

#endif  //AHB_LIB

// strip ahb prefix when non conflict names is assured
#ifdef AHB_STRIP_PREFIX
#define args_init           ahb_args_init
#define args_next           ahb_args_next
#define temp_alloc          ahb_temp_alloc
#define temp_alloc_free     ahb_temp_alloc_free
#define read_entire_file    ahb_read_entire_file
#define is_valid_date       ahb_is_valid_date
#define get_file_size       ahb_get_file_size
#define sb_append           ahb_sb_append
#define sb_to_cstring       ahb_sb_to_cstring
#define split_cstring       ahb_split_cstring
#define TODO                AHB_TODO
#define UNREACHABLE         AHB_UNREACHABLE
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

// pass date = NULL if only want to validate
bool ahb_is_valid_date(const char *date, long long *date_lld)
{
    // accepts only DD-MM-YYYY
    CStr_List l = {0};
    ahb_split(date, '-', &l);
    if (l.count < 3) return false;

    char *day   = l.data[0];
    char *month = l.data[1];
    char *year  = l.data[2];
    if (strlen(day) > 2 || strlen(month) > 2 || strlen(year) > 4) return false;

    int dday   = atoi(day);
    int dmonth = atoi(month);
    int dyear  = atoi(year);
    if (dday <= 0 || dmonth <= 0 || dyear <= 0) return false;
    if (dday > 31 || dmonth > 12 || dyear < 1970) return false;

    if (date_lld) {
        char yyyymmdd[9] = {0};
        sprintf(yyyymmdd, "%4d%02d%02d", dyear, dmonth, dday);
        *date_lld = date_to_lld(yyyymmdd);
    }
    return true;
}

/**
 * @brief Truncate the date into long long
 * 
 * Accepts the date ONLY in the format YYYYMMDD
 * 
 * @param The date in the format YYYYMMDD
 * @return The date in long long
 */
long long date_to_lld(const char *date) {
    char *tmp = temp_alloc(date);
    tmp[8] = '\0';

    char date_[15] = {0};
    strncpy(date_, tmp, 8);
    strncpy(date_+8, tmp+9, 6);
    return atoll(date_);
}


// this funciont allocates memory if sb is empty
void ahb_sb_append(StringBuilder *sb, const char *text)
{
    while (!sb->data || sb->count > sb->capacity) {
        sb->capacity = !sb->data ? 512 : sb->capacity * 2;
        sb->data = realloc(sb->data, sb->capacity);
    }
    memcpy(sb->data + sb->count, text, strlen(text));
    sb->count += strlen(text);
    sb->data[sb->count] = '\0';
}

// this function allocates memory
void ahb_split(const char *cstr, char sep, CStr_List *l)
{
    size_t ini = 0;
    unsigned long i;
    for (i=0; i < strlen(cstr); i++) {
        if (cstr[i] == sep) {
            da_append(l, strndup(cstr+ini, i-ini));
            i++;
            ini = i;
        }
    }
    da_append(l, strndup(cstr+ini, i-ini));
}

#endif // AHB_LIB_IMPLEMENTATION


#include <stdio.h>
#include <dirent.h>
#include <string.h>
#include <stdbool.h>
#include <stdlib.h>
#include <assert.h>

#define AHB_LIB_IMPLEMENTATION
#define AHB_STRIP_PREFIX
#include "ahb_lib.h"

typedef enum {
    ORDER_ASC,
    ORDER_DESC
} Order;

typedef enum {
    ORDERTYPE_DATE,
    ORDERTYPE_PRIORITY
} OrderType;

typedef enum {
    FILTER_NONE,
    FILTER_STATUS_OPEN,
    FILTER_STATUS_CLOSED
} Filter;

void usage() {
    printf("Usage: tatr [OPTIONS]\n");
    printf("OPTIONS:\n");
    printf("  --after    <DD-MM-YYYY>                : filter after a certain date\n");
    printf("  --before   <DD-MM-YYYY>                : gilter before a certain date\n");
    printf("  --priority <STATUS>                    : filter by a certain priority\n");
    printf("  --order  <date, priority> [asc, desc]  : order by date or priority, ascending or descending\n");
    printf("STATUS:\n");
    printf("  OPEN    : opened tasks\n");
    printf("  CLOSED  : closed tasks\n");
    printf("\n");
    printf("Example:\n");
    printf("tatr --after 23-02-2026 --before 23-03-2026 --order date asc --priority OPEN\n");
}

void get_args(int argc, char** argv, OrderType *orderType, Order *order)
{
    State st = {0};
    args_init(&st, argc, argv);

    if (argc == 1) return;

    char *arg;
    while ( (arg = temp_alloc(args_next(&st))) ) {
        if (strcmp(arg, "--order") == 0) {
            arg = temp_alloc(args_next(&st));
            if (arg == NULL) {
                printf("Must inform what to order: 'date' or 'priority'\n");
                exit(1);
            }
            if (strcmp(arg, "date") == 0) {
                *orderType = ORDERTYPE_DATE;
            } else if (strcmp(arg, "priority") == 0) {
                *orderType = ORDERTYPE_PRIORITY;
            } else {
                printf("Invalid order type. Must be 'date' or 'priority'\n");
                exit(1);
            }

            arg = temp_alloc(args_next(&st));
            if (arg == NULL) {
                printf("Must inform order: 'asc' or 'desc'\n");
                exit(1);
            }
            if (strcmp(arg, "asc") == 0) {
                *order = ORDER_ASC;
            } else if (strcmp(arg, "desc") == 0) {
                *order = ORDER_DESC;
            } else {
                printf("Invalid order. Must be 'asc' or 'desc'\n");
                exit(1);
            }
        } else if (strcmp(arg, "--after") == 0) {
            arg = temp_alloc(args_next(&st));
            if (arg == NULL) {
                printf("Must inform a date in the format DD-MM-YYYY\n");
                exit(1);
            }
            if (is_valid_date(arg)) {
                TODO("filter by date with 'after'");
            }

        } else {
            printf("Invalid argument '%s'\n", arg);
            usage();
            exit(1);
        }
    }
}

typedef enum {
    STATUS_OPEN,
    STATUS_CLOSED,
    __status_count
} Status;

typedef struct {
    char *id;
    char *title;
    Status status;
    size_t priority;
} Task;

typedef struct {
    Task *data;
    size_t count;
    size_t capacity;
} TaskList;

char *get_tasks_dir()
{
    DIR *d = NULL;
    struct dirent *dir = NULL;

    char path[256] = {0};
    path[0] = '.';
    int path_idx = 0;

    // TASK(20260408-014434): detect root instead of using maxDepth
    int maxDepth = 5;
    int i = 0;
    bool found = false;
    while (true)
    {
        d = opendir(path);
        while ((dir = readdir(d)) != NULL)
        {
            if (strcmp(dir->d_name, "tasks") == 0)
            {
                found = true;
                break;
            }
        }
        if (found) break;
        
        path[path_idx++] = '.';
        path[path_idx++] = '.';
        path[path_idx++] = '/';
        i++;
        if (i > maxDepth) break;
    }

    if (found) {
        size_t len1 = strlen(path);
        size_t len2 = strlen(dir->d_name);
        char *dirName = malloc(len1 + 1 + len2 + 1);
        memcpy(dirName, path, len1);
        dirName[len1] = '/';
        memcpy(dirName + len1 + 1, dir->d_name, len2);
        dirName[len1 + 1 + len2] = '\0';
        return dirName;
    } else {
        return NULL;
    }

}

// allocates memory in Task->title and Task->id
Task get_task(const char *taskDir, const char *taskId)
{
    StringBuilder sb = {0};
    sb_append(&sb, taskDir);
    sb_append(&sb, "/");
    sb_append(&sb, taskId);
    sb_append(&sb, "/task.md");
    char *path = sb_to_cstring(&sb);

    Task t = {0};
    t.id = strdup(taskId);
    
    char *content = read_entire_file(path);
    bool reading = false;
    bool reading_complete = false;
    size_t beg = -1;
    size_t len = -1;
    size_t count = 0;
    for (size_t i = 0; i < strlen(content); i++) {
        char c = content[i];

        if (c == '#' && !reading) {
            reading = true;
            beg = i + 1;
        }
        if (c == '\n' && reading) {
            reading = false;
            reading_complete = true;
            len = i - beg ;
        }

        if (reading_complete)
        {
            reading_complete = false;
            if (count == 0) // Title
            {
                t.title = malloc(len + 1);
                strncpy(t.title, content + beg, len);
                t.title[len] = '\0';
            }
            else if (count == 1) // Status
            {
                char tmp[256] = {0};
                strncpy(tmp, content + beg, len);
                char* status = strchr(tmp, ':') + 2;
                if (strcmp(status, "OPEN") == 0) {
                    t.status = STATUS_OPEN;
                }
                else if(strcmp(status, "CLOSED") == 0) {
                    t.status = STATUS_CLOSED;
                }
                else {
                    printf("Invalid status '%s' on task '%s'\n", status, taskDir);
                    exit(1);
                }
            }
            else if (count == 2) // Priority
            {
                char tmp[256] = {0};
                strncpy(tmp, content + beg, len);
                char* priority = strchr(tmp, ':') + 1;
                t.priority = atoi(priority);
            }
            count += 1;
        }
    }
    free(content);
    free(path);
    return t;
}

void get_task_list(TaskList *tasks, const char *tasksPath)
{
    DIR *d = NULL;
    struct dirent *dir = NULL;
    d = opendir(tasksPath);
    //int i = 0;
    while ((dir = readdir(d)) != NULL)
    {
        char *taskDir = (dir->d_name);
        if (taskDir[0] == '.') continue;

        Task t = get_task(tasksPath, taskDir); // remember to free t.title
        da_append(tasks, t);
    }
}

const char *status_to_cstring(Status st)
{
    static_assert(__status_count == 2, "missing some enum Status");
    if      (st == STATUS_OPEN)   return "Open";
    else if (st == STATUS_CLOSED) return "Closed";
    else UNREACHABLE("status_to_cstring");
}

void print_task_list(TaskList tasks, char *taskDir) 
{
    for (size_t i=0; i < tasks.count; i++) {
        Task t = tasks.data[i];
        StringBuilder sb = {0};
        sb_append(&sb, taskDir);
        sb_append(&sb, "/");
        sb_append(&sb, t.id);
        sb_append(&sb, "/task.md");
        char *path = sb_to_cstring(&sb);

        printf("%s:3:11: title: %-40.40s  | status: %-6.6s | priority: %3zu\n", path, t.title, status_to_cstring(t.status), t.priority);
        free(path);
    }
}

#define compare_priority_body(num)               \
    const Task *aa = a;                          \
    const Task *bb = b;                          \
    if (aa->priority > bb->priority) return num;   \
    if (aa->priority < bb->priority) return -num;  \
    return 0;
int compare_priority_asc(const void *a, const void *b)
{
    compare_priority_body(1);
}
int compare_priority_desc(const void *a, const void *b)
{
    compare_priority_body(-1);
}

long long date_to_lld(const char *date) {
    char *tmp = temp_alloc(date);
    tmp[8] = '\0';

    char date_[15] = {0};
    strncpy(date_, tmp, 8);
    strncpy(date_+8, tmp+9, 6);
    return atoll(date_);
}

#define compare_date_body(num)                               \
    const Task *aa = a;                                      \
    const Task *bb = b;                                      \
    if (!aa->id || !bb->id) return 0;                        \
                                                             \
    long long da = date_to_lld(aa->id);                      \
    long long db = date_to_lld(bb->id);                      \
                                                             \
    if (da > db) return num;                                 \
    if (da < db) return -num;                                \
    return 0;
int compare_date_asc(const void *a, const void *b)
{
    compare_date_body(1);
}
int compare_date_desc(const void *a, const void *b)
{
    compare_date_body(-1);
}


int main(int argc, char **argv)
{
    OrderType orderType = ORDERTYPE_PRIORITY;
    Order order = ORDER_DESC;
    Filter filter = FILTER_NONE;
    get_args(argc, argv, &orderType, &order);

    bool is_desc = order == ORDER_DESC;
    printf("filter: %s\n", FILTER_STATUS_OPEN ? "OPEN" : "NONE");
    printf("order type: %s\n", orderType == ORDERTYPE_PRIORITY ? "priority" : "date");
    printf("order: %s\n", is_desc ? "desc" : "asc");

    char *tasksPath = get_tasks_dir();
    if (!tasksPath) {
        printf("Task directory was not found. Please create one\n");
        exit(1);
    }

    // getting all tasks from task folder
    TaskList allTasks = {0};
    get_task_list(&allTasks, tasksPath);

    TaskList tasks = {0};
    filter = FILTER_STATUS_CLOSED; // TODO(Andre): get filter from arguments
    if (filter != FILTER_NONE) {
        Status st = -1;

        for (size_t i=0; i < allTasks.count; i++) {
            Task t = allTasks.data[i];

            static_assert(__status_count == 2, "missing some enum Status");
            if      (filter == FILTER_STATUS_OPEN)   st = STATUS_OPEN;
            else if (filter == FILTER_STATUS_CLOSED) st = STATUS_CLOSED;
            else UNREACHABLE("match filter to status");
            assert((int)st != -1 && "match filter to status");
            
            printf("%s\n", status_to_cstring(t.status));
            if (t.status == st) {
                da_append(&tasks, t);
            }
        }
    } else {
        tasks = allTasks;
    }


    // sorting list
    if (orderType == ORDERTYPE_PRIORITY) 
        qsort(tasks.data, tasks.count, sizeof(tasks.data[0]), is_desc ? compare_priority_desc : compare_priority_asc);
    else if (orderType == ORDERTYPE_DATE)
        qsort(tasks.data, tasks.count, sizeof(tasks.data[0]), is_desc ? compare_date_desc : compare_date_asc);
    
    // show processed list
    print_task_list(tasks, tasksPath);

    temp_alloc_free();
    return 0;
}

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
    ORDER_DESC,
    ORDER_ASC
} Order;

typedef enum {
    ORDERTYPE_PRIORITY,
    ORDERTYPE_DATE
} OrderType;

typedef enum {
    FILTER_NONE,
    FILTER_STATUS_OPEN,
    FILTER_STATUS_CLOSED,
    __filter_count
} Filter;

typedef struct {
    Order order;
    OrderType orderType;
    Filter filter;
} CmdArgs;

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

void usage() {
    printf("Usage: tatr [OPTIONS]\n");
    printf("OPTIONS:\n");
    printf("  --after    <DD-MM-YYYY>                : filter after a certain date\n");
    printf("  --before   <DD-MM-YYYY>                : filter before a certain date\n");
    printf("  --status   <STATUS>                    : filter by a certain status\n");
    printf("  --order  <date, priority> [asc, desc]  : order by date or priority, ascending or descending\n");
    printf("STATUS:\n");
    static_assert(__status_count == 2, "missing enum Status");
    printf("  open    : opened tasks\n");
    printf("  closed  : closed tasks\n");
    printf("\n");
    printf("Example:\n");
    printf("  tatr --after 23-02-2026 --before 23-03-2026 --order date asc --status OPEN\n");
    printf("NOTE: Order will be executed last. Filters are executed in the order they are\n");
    printf("      passed in the arguments.\n");
}

void get_args(int argc, char** argv, CmdArgs *args)
{
    State st = {0};
    args_init(&st, argc, argv);

    if (argc == 1) return;

    char *arg;
    while ( (arg = temp_alloc(args_next(&st))) ) {
        if (strcmp(arg, "--order") == 0) {
            arg = temp_alloc(args_next(&st));
            if (arg == NULL) {
                ABORT("Must inform what to order: 'date' or 'priority'");
            }
            if (strcmp(arg, "date") == 0) {
                args->orderType = ORDERTYPE_DATE;
            } else if (strcmp(arg, "priority") == 0) {
                args->orderType = ORDERTYPE_PRIORITY;
            } else {
                ABORT("Invalid order type. Must be 'date' or 'priority'");
            }

            arg = temp_alloc(args_next(&st));
            if (arg == NULL) {
                ABORT("Must inform order: 'asc' or 'desc'");
            }
            if (strcmp(arg, "asc") == 0) {
                args->order = ORDER_ASC;
            } else if (strcmp(arg, "desc") == 0) {
                args->order = ORDER_DESC;
            } else {
                ABORT("Invalid order. Must be 'asc' or 'desc'");
            }
        } else if (strcmp(arg, "--status") == 0) {
            static_assert(__status_count == 2, "missing enum Status");
            arg = temp_alloc(args_next(&st));
            if (arg == NULL) {
                ABORT("Must inform a status 'open' or 'closed'");
            }
            if (strcmp(arg, "open") == 0) {
                args->filter = FILTER_STATUS_OPEN;
            } else if (strcmp(arg, "closed") == 0) {
                args->filter = FILTER_STATUS_CLOSED;
            } else {
                ABORT("Invalid status. Must be 'open' or 'closed'");
            }
        } else if (strcmp(arg, "--after") == 0) {
            arg = temp_alloc(args_next(&st));
            if (arg == NULL) {
                ABORT("Must inform a date in the format DD-MM-YYYY");
            }
            Date date = {0};
            if (is_valid_date(arg, &date)) {
                TODO("filter by date with 'after'");
            } else {
                printf("ERROR: Invalid date '%s'\n", arg);
                exit(1);
            }

        } else {
            printf("ERROR: Invalid argument '%s'\n", arg);
            usage();
            exit(1);
        }
    }
}


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
    char *path = sb.data;

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

const char *filter_to_cstring(Filter ft)
{
    static_assert(__filter_count == 3, "missing some enum Filter");
    if      (ft == FILTER_NONE)          return "none";
    else if (ft == FILTER_STATUS_CLOSED) return "closed";
    else if (ft == FILTER_STATUS_OPEN)   return "open";
    else UNREACHABLE("filter_to_cstring");
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
        char *path = sb.data;

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
    CmdArgs args = {0}; // default: order by priority desc with no filter
    get_args(argc, argv, &args);

    bool is_desc = args.order == ORDER_DESC;
    printf("Filtered by %s. %s order by %s\n", filter_to_cstring(args.filter), is_desc ? "Descending" : "Ascending", args.orderType == ORDERTYPE_PRIORITY ? "priority" : "date");

    char *tasksPath = get_tasks_dir();
    if (!tasksPath) {
        printf("Task directory was not found. Please create one\n");
        exit(1);
    }

    // getting all tasks from task folder
    TaskList allTasks = {0};
    get_task_list(&allTasks, tasksPath);

    TaskList tasks = {0};
    if (args.filter != FILTER_NONE) {
        Status st = -1;

        for (size_t i=0; i < allTasks.count; i++) {
            Task t = allTasks.data[i];

            static_assert(__status_count == 2, "missing some enum Status");
            if      (args.filter == FILTER_STATUS_OPEN)   st = STATUS_OPEN;
            else if (args.filter == FILTER_STATUS_CLOSED) st = STATUS_CLOSED;
            else UNREACHABLE("match filter to status");
            assert((int)st != -1 && "match filter to status");
            
            if (t.status == st) {
                da_append(&tasks, t);
            }
        }
    } else {
        // no filter was selected
        tasks = allTasks;
    }

    // sorting list
    if (args.orderType == ORDERTYPE_PRIORITY) 
        qsort(tasks.data, tasks.count, sizeof(tasks.data[0]), is_desc ? compare_priority_desc : compare_priority_asc);
    else if (args.orderType == ORDERTYPE_DATE)
        qsort(tasks.data, tasks.count, sizeof(tasks.data[0]), is_desc ? compare_date_desc : compare_date_asc);
    
    // show processed list
    print_task_list(tasks, tasksPath);

    temp_alloc_free();
    return 0;
}

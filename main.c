#include <stdio.h>
#include <dirent.h>
#include <string.h>
#include <stdbool.h>
#include <stdlib.h>

#define AHB_LIB_IMPLEMENTATION
#define AHB_STRIP_PREFIX
#include "ahb_lib.h"

typedef enum {
    ASC,
    DESC
} Order;

typedef enum {
    DATE,
    PRIORITY
} OrderType;

void usage() {
    printf("Usage: tatr [--order] <date, priority> [asc, desc]\n");
}

void get_args(int argc, char** argv, OrderType *orderType, Order *order)
{
    State st = {0};
    args_init(&st, argc, argv);

    if (argc == 1) return;

    char *arg = temp_alloc(args_next(&st));
    if (strcmp(arg, "--order") == 0) {
        arg = temp_alloc(args_next(&st));
        if (arg == NULL) {
            printf("Must inform what to order: 'date' or 'priority'\n");
            exit(1);
        }
        if (strcmp(arg, "date") == 0) {
            *orderType = DATE;
        } else if (strcmp(arg, "priority") == 0) {
            *orderType = PRIORITY;
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
            *order = ASC;
        } else if (strcmp(arg, "desc") == 0) {
            *order = DESC;
        } else {
            printf("Invalid order. Must be 'asc' or 'desc'\n");
            exit(1);
        }
    } else {
        printf("Invalid argument '%s'\n", arg);
        usage();
        exit(1);
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

// this allocates memory
char *read_entire_file(const char *path)
{
    FILE *f = fopen(path, "r");
    assert(f && "Could not open file");
    fseek(f, 0L, SEEK_END);
    size_t size = ftell(f);
    rewind(f);
    char *content = malloc(size + 1);
    assert(content && "Could not allocate memoryu. Buy more RAM.");
    int count = fread(content, 1, size, f);
    content[size] = '\0';
    assert((size_t)count == size && "Could not read entire file");
    fclose(f);
    return content;
}

typedef enum {
    OPEN,
    CLOSED
} Status;

typedef struct {
    char *id;
    char *title;
    Status status;
    size_t priority;
} Task;

// allocates memory in Task->title and Task->id
Task get_task(const char *taskDir, const char *taskId)
{

    // TASK(20260415-172036): implement a string builder
    char path[256] = {0};
    strcat(path, taskDir);
    strcat(path, "/");
    strcat(path, taskId);
    strcat(path, "/task.md");

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
                    t.status = OPEN;
                }
                else if(strcmp(status, "CLOSED") == 0) {
                    t.status = CLOSED;
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
    return t;
}

void get_task_list(Task *tasks, const char *tasksPath)
{
    DIR *d = NULL;
    struct dirent *dir = NULL;
    d = opendir(tasksPath);
    int i = 0;
    while ((dir = readdir(d)) != NULL)
    {
        char *taskDir = (dir->d_name);
        if (taskDir[0] == '.') continue;

        // TASK(20260417-170612): add tasks to a dynamic array
        Task t = get_task(tasksPath, taskDir); // remember to free t.title
        tasks[i++] = t;
    }
}

void print_task_list(Task *tasks, char *taskDir) 
{

    for (int i=0; i < 256; i++) {
        Task t = tasks[i];
        if (!t.title) continue;

        char path[256] = {0};
        strcat(path, taskDir);
        strcat(path, "/");
        strcat(path, t.id);
        strcat(path, "/task.md");

        printf("%s:3:11: title: %-40.40s  | priority: %3zu\n", path, t.title, t.priority);
    }
}

int compare_priority_asc(const void *a, const void *b)
{
    const Task *aa = a;
    const Task *bb = b;
    if (aa->priority > bb->priority) return 1;
    if (aa->priority < bb->priority) return -1;
    return 0;
}
int compare_priority_desc(const void *a, const void *b)
{
    const Task *aa = a;
    const Task *bb = b;
    if (aa->priority > bb->priority) return -1;
    if (aa->priority < bb->priority) return 1;
    return 0;
}
int compare_date_asc(const void *a, const void *b)
{
    const Task *aa = a;
    const Task *bb = b;
    if (!aa->id || !bb->id) return 0;

    char *tmpa = temp_alloc(aa->id);
    char *tmpb = temp_alloc(bb->id);
    tmpa[8] = '\0'; tmpb[8] = '\0';

    char dta[9] = {0};
    char dtb[9] = {0};
    char tma[7] = {0};
    char tmb[7] = {0};
    strcpy(dta, tmpa);
    strcpy(dtb, tmpb);
    strcpy(tma, tmpa+9);
    strcpy(tmb, tmpb+9);

    char datea[15] = {0};
    char dateb[15] = {0};
    strcat(datea, dta);
    strcat(datea, tma);
    strcat(dateb, dtb);
    strcat(dateb, tmb);

    long long da = atoll(datea);
    long long db = atoll(dateb);

    if (da > db) return 1;
    if (da < db) return -1;
    return 0;
}
int compare_date_desc(const void *a, const void *b)
{
    const Task *aa = a;
    const Task *bb = b;
    if (!aa->id || !bb->id) return 0;

    char *tmpa = temp_alloc(aa->id);
    char *tmpb = temp_alloc(bb->id);
    tmpa[8] = '\0'; tmpb[8] = '\0';

    char dta[9] = {0};
    char dtb[9] = {0};
    char tma[7] = {0};
    char tmb[7] = {0};
    strcpy(dta, tmpa);
    strcpy(dtb, tmpb);
    strcpy(tma, tmpa+9);
    strcpy(tmb, tmpb+9);

    char datea[15] = {0};
    char dateb[15] = {0};
    strcat(datea, dta);
    strcat(datea, tma);
    strcat(dateb, dtb);
    strcat(dateb, tmb);

    long long da = atoll(datea);
    long long db = atoll(dateb);

    if (da > db) return -1;
    if (da < db) return 1;
    return 0;
}

int main(int argc, char **argv)
{
    OrderType orderType = PRIORITY;
    Order order = DESC;
    get_args(argc, argv, &orderType, &order);

    char *tasksPath = get_tasks_dir();
    if (!tasksPath) {
        printf("Task directory was not found. Please create one\n");
        exit(1);
    }

    // listing all tasks from task folder
    Task tasks[256] = {0};
    get_task_list(tasks, tasksPath);

    // TODO: implement the sorting and filtering algorithms
    bool is_desc = order == DESC;
    if (orderType == PRIORITY) 
        qsort(tasks, 256, sizeof(tasks[0]), is_desc ? compare_priority_desc : compare_priority_asc);
    else if (orderType == DATE)
        qsort(tasks, 256, sizeof(tasks[0]), is_desc ? compare_date_desc : compare_date_asc);
    
    printf("order type: %s\n", orderType == PRIORITY ? "priority" : "date");
    printf("order: %s\n", is_desc ? "desc" : "asc");
    print_task_list(tasks, tasksPath);

    temp_alloc_free();
    return 0;
}

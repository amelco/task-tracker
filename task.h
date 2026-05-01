#ifndef TASKS_H_
#define TASKS_H_

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


#endif // TASKS_H_

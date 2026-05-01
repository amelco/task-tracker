#include <stdio.h>
#include <dirent.h>
#include <string.h>
#include <stdbool.h>
#include <stdlib.h>
#include <assert.h>

#define AHB_LIB_IMPLEMENTATION
#define AHB_STRIP_PREFIX
#include "ahb_lib.h"

#include "task.h"
#include "args.h"

const char *filter_to_cstring(Filter ft)
{
    static_assert(__filter_count == 4, "missing some enum Filter");
    if      (ft == FILTER_NONE)          return "none";
    else if (ft == FILTER_STATUS_CLOSED) return "closed";
    else if (ft == FILTER_STATUS_OPEN)   return "open";
    else if (ft == FILTER_AFTER)         return "after";
    else UNREACHABLE("filter_to_cstring");
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
        Status status = -1;

        for (size_t i=0; i < allTasks.count; i++) {
            Task t = allTasks.data[i];

            // Filtering by status first
            static_assert(__status_count == 2, "missing some enum Status");
            if      (args.filter == FILTER_STATUS_OPEN)   status = STATUS_OPEN;
            else if (args.filter == FILTER_STATUS_CLOSED) status = STATUS_CLOSED;
            else UNREACHABLE("match filter to status");
            assert((int)status != -1 && "match filter to status");
            
            if (t.status == status) {
                da_append(&tasks, t);
            }

            // Filtering by date 
            if (args.filter == FILTER_AFTER) {
                // TODO: only add tasks where id is greater than the date
                // if (t.id > after_date) da_append(&tasks, t);
                // - need to get the after_date from the args
                // - need to convert id to long long (YYYYMMDD)
            }
            //if (args.filter == FILTER_BEFORE) {
            //}
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

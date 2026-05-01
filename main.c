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

    bool statusFilter = args.filterStatus != FILTER_STATUS_NONE;
    bool dateFilter = args.filterDate != FILTER_DATE_NONE;
    bool is_desc = args.order == ORDER_DESC;

    // getting all tasks from task folder
    char *tasksPath = get_tasks_dir();
    if (!tasksPath) {
        printf("Task directory was not found. Please create one\n");
        exit(1);
    }
    TaskList allTasks = {0};
    get_task_list(&allTasks, tasksPath);

    TaskList tasks = {0};

    if (statusFilter || dateFilter) {
        Status status = -1;

        for (size_t i=0; i < allTasks.count; i++) {
            Task t = allTasks.data[i];

            // Filtering by status
            // It's ok to have no status filter.
            static_assert(__status_count == 2, "missing some enum Status");
            if      (args.filterStatus == FILTER_STATUS_OPEN)   status = STATUS_OPEN;
            else if (args.filterStatus == FILTER_STATUS_CLOSED) status = STATUS_CLOSED;
            
            if (!dateFilter && t.status == status) {
                da_append(&tasks, t);
            }

            // Filtering by date 
            if (dateFilter) {
                if (args.filterDate == 1) {
                    CStr_List l = {0};
                    split(t.id, '-', &l);
                    long long date_task = date_to_lld(l.data[0]);
                    free(l.data);
                    long long date_after = args.filterDateData.date_after;
                    if (date_task > date_after) {
                        if (t.status == status || !statusFilter) {
                            da_append(&tasks, t);
                        }
                    }
                }
                else if (args.filterDate == 2) {
                    CStr_List l = {0};
                    split(t.id, '-', &l);
                    long long date_task = date_to_lld(l.data[0]);
                    free(l.data);
                    long long date_before = args.filterDateData.date_before;
                    if (date_task < date_before) {
                        if (t.status == status || !statusFilter) {
                            da_append(&tasks, t);
                        }
                    }
                }
                else if (args.filterDate == 3) {
                    CStr_List l = {0};
                    split(t.id, '-', &l);
                    long long date_task = date_to_lld(l.data[0]);
                    free(l.data);
                    long long date_before = args.filterDateData.date_before;
                    long long date_after = args.filterDateData.date_after;
                    if (date_task < date_before && date_task > date_after) {
                        if (t.status == status || !statusFilter) {
                            da_append(&tasks, t);
                        }
                    }
                }
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

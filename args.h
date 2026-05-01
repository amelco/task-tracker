#ifndef ARGS_H_
#define ARGS_H_

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
    FILTER_AFTER,
    __filter_count
} Filter;

typedef struct {
    Order order;
    OrderType orderType;
    Filter filter;
} CmdArgs;

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
    printf("NOTE: Order command will be executed last. Filters are executed in the following order: status, after and before\n");
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
                args->filter |= FILTER_STATUS_OPEN;
            } else if (strcmp(arg, "closed") == 0) {
                args->filter |= FILTER_STATUS_CLOSED;
            } else {
                ABORT("Invalid status. Must be 'open' or 'closed'");
            }
        } else if (strcmp(arg, "--after") == 0) {
            arg = temp_alloc(args_next(&st));
            if (arg == NULL) {
                ABORT("Must inform a date in the format DD-MM-YYYY");
            }
            long long date;
            if (is_valid_date(arg, &date)) {
                args->filter |= FILTER_AFTER;
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

#endif // ARGS_H_

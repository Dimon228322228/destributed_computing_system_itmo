#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "child.h"
#include "log.h"
#include "parent.h"
#include "pipes.h"
#include "process.h"

static int parse_args(int argc, char *argv[], process_context *ctx) {
    int i;
    int child_count = -1;
    int bal_i = 0;

    for (i = 1; i < argc; ++i) {
        if (strcmp(argv[i], "-p") == 0) {
            if (i + 1 >= argc) return -1;
            child_count = atoi(argv[i + 1]);
            i += 2;
            break;
        }
    }

    if (child_count < 1 || child_count > MAX_PROCESS_ID) return -1;

    if (i + child_count > argc) return -1;

    for (bal_i = 1; bal_i <= child_count; ++bal_i) {
        ctx->start_balances[bal_i] = (balance_t)atoi(argv[i + bal_i - 1]);
    }

    ctx->n = child_count + 1;
    return child_count;
}

int main(int argc, char *argv[])
{
    process_context ctx;
    int role;

    memset(&ctx, 0, sizeof(ctx));
    ctx.id = PARENT_ID;

    if (parse_args(argc, argv, &ctx) < 0) return 1;

    if (log_init() != 0) return 1;

    if (create_pipes(&ctx) != 0) {
        log_close();
        return 1;
    }

    if (log_pipes(&ctx) != 0) {
        log_close();
        return 1;
    }

    role = start_processes(&ctx);
    if (role < 0) {
        log_close();
        return 1;
    }

    if (role == PROCESS_ROLE_CHILD) {
        if (run_child(&ctx) != 0) {
            log_close();
            return 1;
        }
    } else {
        if (run_parent(&ctx) != 0) {
            log_close();
            return 1;
        }
    }

    log_close();
    return 0;
}

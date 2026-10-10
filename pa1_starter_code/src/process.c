#include <unistd.h>

#include "pipes.h"
#include "process.h"

int start_processes(process_context *ctx) {
    local_id child_id;
    pid_t pid;

    for (child_id = 1; child_id < ctx->n; ++child_id) {
        pid = fork();
        if (pid < 0) return -1;

        if (pid == 0) {
            ctx->id = child_id;
            close_unused_pipes(ctx);
            return PROCESS_ROLE_CHILD;
        }
    }

    ctx->id = PARENT_ID;
    close_unused_pipes(ctx);
    return PROCESS_ROLE_PARENT;
}
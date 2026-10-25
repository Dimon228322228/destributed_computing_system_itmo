#include <sys/types.h>
#include <unistd.h>

#include "pipes.h"
#include "process.h"
#include "lamport.h"

int start_processes(process_context *ctx) {
    for (local_id child_id = 1; child_id < ctx->n; ++child_id) {
        pid_t pid = fork();
        if (pid < 0) return -1;

        if (pid == 0) {
            ctx->id = child_id;
            close_unused_pipes(ctx);
            lamport_set_context(ctx);
            return PROCESS_ROLE_CHILD;
        }
    }

    ctx->id = PARENT_ID;
    close_unused_pipes(ctx);
    lamport_set_context(ctx);
    return PROCESS_ROLE_PARENT;
}

#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

#include "ipc.h"
#include "parent.h"

static int receive_from_all_children(process_context *ctx, MessageType type) {
    Message msg;
    local_id from;

    for (from = 1; from < ctx->n; ++from) {
        memset(&msg, 0, sizeof(msg));
        if (receive(ctx, from, &msg) != 0) return -1;
        if (msg.s_header.s_type != type) return -1;
    }

    return 0;
}

int run_parent(process_context *ctx) {
    int i;
    int status;
    if (receive_from_all_children(ctx, STARTED) != 0) return -1;
    if (receive_from_all_children(ctx, DONE) != 0) return -1;
    for (i = 1; i < ctx->n; ++i) 
        if (wait(&status) < 0) 
            return -1;
    return 0;
}

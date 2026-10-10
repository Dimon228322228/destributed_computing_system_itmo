#include <stdio.h>
#include <unistd.h>

#include "common.h"
#include "pipes.h"

int create_pipes(process_context *ctx) {
    local_id from;
    local_id to;
    int fd[2];

    for (from = 0; from < ctx->n; ++from) {
        for (to = 0; to < ctx->n; ++to) {
            if (from == to) {
                ctx->pipes[from][to].rd = -1;
                ctx->pipes[from][to].wr = -1;
                continue;
            }
            if (pipe(fd) != 0) return -1;
            ctx->pipes[from][to].rd = fd[0];
            ctx->pipes[from][to].wr = fd[1];
        }
    }
    return 0;
}

int log_pipes(const process_context *ctx) {
    FILE *file;
    local_id from;
    local_id to;

    file = fopen(pipes_log, "w");
    if (file == NULL) return -1;

    for (from = 0; from < ctx->n; ++from) {
        for (to = 0; to < ctx->n; ++to) {
            if (from == to) continue;
            fprintf(file, "pipe %d -> %d: rd=%d wr=%d\n", from, to, ctx->pipes[from][to].rd, ctx->pipes[from][to].wr);
        }
    }

    fclose(file);
    return 0;
}

void close_unused_pipes(process_context *ctx) {
    local_id from;
    local_id to;
    local_id me = ctx->id;

    for (from = 0; from < ctx->n; ++from) {
        for (to = 0; to < ctx->n; ++to) {
            if (from == to) continue;

            if (from == me) { // write to this pipe so there is no need to read from it
                if (ctx->pipes[from][to].rd != -1) {
                    close(ctx->pipes[from][to].rd);
                    ctx->pipes[from][to].rd = -1;
                }
            } else if (to == me) { // read from here so there is no need to write to it
                if (ctx->pipes[from][to].wr != -1) {
                    close(ctx->pipes[from][to].wr);
                    ctx->pipes[from][to].wr = -1;
                }
            } else {
                if (ctx->pipes[from][to].rd != -1) {
                    close(ctx->pipes[from][to].rd);
                    ctx->pipes[from][to].rd = -1;
                }
                if (ctx->pipes[from][to].wr != -1) {
                    close(ctx->pipes[from][to].wr);
                    ctx->pipes[from][to].wr = -1;
                }
            }
        }
    }
}
#ifndef PROCESS_H
#define PROCESS_H

#include "pa2_io.h"

typedef enum {
    PROCESS_ROLE_PARENT = 0,
    PROCESS_ROLE_CHILD
} process_role;

/*
 * Create children and per each one set ctx->id then close unused pipes.
 * Return ROLE of current thread, otherwise -1 that means FAIL.
 */
int start_processes(process_context *ctx);

#endif

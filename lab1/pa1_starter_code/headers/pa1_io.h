#ifndef PA1_IO_H
#define PA1_IO_H

#include "ipc.h"

typedef struct {
    int rd;
    int wr;
} pipe_ends;

typedef struct {
    local_id id;
    int n;
    pipe_ends pipes[MAX_PROCESS_ID + 1][MAX_PROCESS_ID + 1];
} process_context;

#endif

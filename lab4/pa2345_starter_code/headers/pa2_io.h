#ifndef PA2_IO_H
#define PA2_IO_H

#include "ipc.h"

typedef struct {
    int rd;
    int wr;
} pipe_ends;

typedef struct {
    timestamp_t time;
    local_id    id;
} cs_queue_entry;

typedef struct {
    local_id id;
    int n;
    pipe_ends pipes[MAX_PROCESS_ID + 1][MAX_PROCESS_ID + 1];
    timestamp_t lamport_time;
    int use_mutex; /* 1 if --mutexl */
    cs_queue_entry cs_queue[MAX_PROCESS_ID + 1];
    int cs_queue_len;
    timestamp_t cs_req_time;
    int cs_replied[MAX_PROCESS_ID + 1];
} process_context;

/* 0 — SUCCESS, 1 — WAIT or EMPTY, -1 — FAIL */
int send_nowait(void *self, local_id dst, const Message *msg);
int receive_any_nowait(void *self, Message *msg);
int receive_any_nowait_from(void *self, Message *msg, local_id *from);

#endif

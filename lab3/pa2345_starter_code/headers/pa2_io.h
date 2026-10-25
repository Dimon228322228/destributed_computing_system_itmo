#ifndef PA2_IO_H
#define PA2_IO_H

#include "banking.h"
#include "ipc.h"

typedef struct {
    int rd;
    int wr;
} pipe_ends;

typedef struct {
    local_id id;
    int n;
    pipe_ends pipes[MAX_PROCESS_ID + 1][MAX_PROCESS_ID + 1];
    balance_t start_balances[MAX_PROCESS_ID + 1];
    balance_t balance;
    BalanceHistory history;
    timestamp_t lamport_time;
} process_context;

/* 0 — SUCCESS, 1 — WAIT or EMPTY, -1 — FAIL */
int send_nowait(void *self, local_id dst, const Message *msg);
int receive_any_nowait(void *self, Message *msg);

#endif

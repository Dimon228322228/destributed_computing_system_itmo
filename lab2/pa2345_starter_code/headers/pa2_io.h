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
} process_context;

#endif
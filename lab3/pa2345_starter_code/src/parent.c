#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

#include "banking.h"
#include "ipc.h"
#include "parent.h"
#include "lamport.h"

static int receive_from_all_children(process_context *ctx, MessageType type) {
    Message msg;

    for (local_id from = 1; from < ctx->n; ++from) {
        memset(&msg, 0, sizeof(msg));
        if (receive(ctx, from, &msg) != 0) return -1;
        if (msg.s_header.s_type != type) return -1;
    }

    return 0;
}

static int send_stop_to_children(process_context *ctx) {
    Message msg;
    memset(&msg, 0, sizeof(msg));
    lamport_tick(ctx);
    msg.s_header.s_magic = MESSAGE_MAGIC;
    msg.s_header.s_type = STOP;
    msg.s_header.s_local_time = get_lamport_time();
    msg.s_header.s_payload_len = 0;
    for (local_id dst = 1; dst < ctx->n; ++dst) 
        if (send(ctx, dst, &msg) != 0) return -1;
    return 0;
}

static int collect_history(process_context *ctx, AllHistory *all) {
    Message msg;
    memset(all, 0, sizeof(*all));
    all->s_history_len = (uint8_t)(ctx->n - 1);

    for (local_id from = 1; from < ctx->n; ++from) {
        memset(&msg, 0, sizeof(msg));
        if (receive(ctx, from, &msg) != 0) return -1;
        if (msg.s_header.s_type != BALANCE_HISTORY) return -1;
        if (msg.s_header.s_payload_len == 0) return -1;
        if (msg.s_header.s_payload_len > sizeof(BalanceHistory)) return -1;
        memcpy(&all->s_history[from - 1], msg.s_payload, msg.s_header.s_payload_len);
    }
    return 0;
}

int run_parent(process_context *ctx) {
    AllHistory all;
    local_id max_id = (local_id)(ctx->n - 1);
    if (receive_from_all_children(ctx, STARTED) != 0) return -1;
    bank_robbery(ctx, max_id);
    if (send_stop_to_children(ctx) != 0) return -1;
    if (receive_from_all_children(ctx, DONE) != 0) return -1;
    if (collect_history(ctx, &all) != 0) return -1;
    print_history(&all);
    for (int i = 1; i < ctx->n; ++i) 
        if (wait(NULL) < 0) return -1;
    return 0;
}

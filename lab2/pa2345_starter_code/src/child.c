#include <stdio.h>
#include <stddef.h>
#include <string.h>
#include <unistd.h>
#include "banking.h"
#include "child.h"
#include "history.h"
#include "ipc.h"
#include "log.h"
#include "pa2345.h"

static int wait_from_other_children(process_context *ctx, MessageType type) {
    Message msg;
    local_id from;

    for (from = 1; from < ctx->n; ++from) {
        if (from == ctx->id) continue;

        memset(&msg, 0, sizeof(msg));
        if (receive(ctx, from, &msg) != 0) return -1;
        if (msg.s_header.s_type != type) return -1;
    }

    return 0;
}

static int wait_started_from_others(process_context *ctx) {
    return wait_from_other_children(ctx, STARTED);
}

static int send_payload_multicast(process_context *ctx, MessageType type, const char *text) {
    Message msg;
    size_t len;

    memset(&msg, 0, sizeof(msg));
    len = strlen(text);

    msg.s_header.s_magic = MESSAGE_MAGIC;
    msg.s_header.s_payload_len = (uint16_t)len;
    msg.s_header.s_type = (int16_t)type;
    msg.s_header.s_local_time = get_physical_time();
    memcpy(msg.s_payload, text, len);

    return send_multicast(ctx, &msg);
}

static int handle_transfer(process_context *ctx, Message *msg) {
    TransferOrder order;
    timestamp_t t;
    char buf[MAX_PAYLOAD_LEN];
    Message ack;
    memcpy(&order, msg->s_payload, sizeof(order));
    t = get_physical_time();
    if (order.s_src == ctx->id) {
        ctx->balance -= order.s_amount;
        update_history(&ctx->history, ctx->balance, t);
        snprintf(buf, sizeof(buf), log_transfer_out_fmt, t, ctx->id, order.s_amount, order.s_dst);
        log_event(buf);
        msg->s_header.s_local_time = t;
        if (send(ctx, order.s_dst, msg) != 0) return -1;
    } else if (order.s_dst == ctx->id) {
        ctx->balance += order.s_amount;
        update_history(&ctx->history, ctx->balance, t);
        snprintf(buf, sizeof(buf), log_transfer_in_fmt, t, ctx->id, order.s_amount, order.s_src);
        log_event(buf);
        memset(&ack, 0, sizeof(ack));
        ack.s_header.s_magic = MESSAGE_MAGIC;
        ack.s_header.s_type = ACK;
        ack.s_header.s_local_time = t;
        ack.s_header.s_payload_len = 0;
        if (send(ctx, PARENT_ID, &ack) != 0) return -1;
    }
    return 0;
}

static int work_until_stop(process_context *ctx) {
    Message msg;
    for (;;) {
        memset(&msg, 0, sizeof(msg));
        if (receive_any(ctx, &msg) != 0) return -1;
        if (msg.s_header.s_type == TRANSFER) {
            if (handle_transfer(ctx, &msg) != 0) return -1;
        } else if (msg.s_header.s_type == STOP) {
            return 0;
        }
    }
}

/* после STOP: шлём DONE, ждём DONE от других детей, по пути ещё TRANSFER */
static int done_phase(process_context *ctx) {
    Message msg;
    char buf[MAX_PAYLOAD_LEN];
    int need;
    int got;
    timestamp_t t;
    t = get_physical_time();
    snprintf(buf, sizeof(buf), log_done_fmt, t, ctx->id, ctx->balance);
    log_event(buf);
    if (send_payload_multicast(ctx, DONE, buf) != 0) return -1;
    need = ctx->n - 2; /* остальные дети */
    got = 0;
    while (got < need) {
        memset(&msg, 0, sizeof(msg));
        if (receive_any(ctx, &msg) != 0) return -1;
        if (msg.s_header.s_type == TRANSFER) {
            if (handle_transfer(ctx, &msg) != 0) return -1;
        } else if (msg.s_header.s_type == DONE) {
            got++;
        }
    }
    t = get_physical_time();
    snprintf(buf, sizeof(buf), log_received_all_done_fmt, t, ctx->id);
    log_event(buf);
    return 0;
}

static int send_balance_history(process_context *ctx) {
    Message msg;
    uint16_t len;
    update_history(&ctx->history, ctx->balance, get_physical_time());
    memset(&msg, 0, sizeof(msg));
    len = (uint16_t)(offsetof(BalanceHistory, s_history) + sizeof(BalanceState) * ctx->history.s_history_len);
    msg.s_header.s_magic = MESSAGE_MAGIC;
    msg.s_header.s_type = BALANCE_HISTORY;
    msg.s_header.s_local_time = get_physical_time();
    msg.s_header.s_payload_len = len;
    memcpy(msg.s_payload, &ctx->history, len);
    return send(ctx, PARENT_ID, &msg);
}

int run_child(process_context *ctx) {
    char buf[MAX_PAYLOAD_LEN];
    timestamp_t t;
    ctx->balance = ctx->start_balances[ctx->id];
    ctx->history.s_id = ctx->id;
    ctx->history.s_history_len = 0;
    update_history(&ctx->history, ctx->balance, get_physical_time());
    t = get_physical_time();
    snprintf(buf, sizeof(buf), log_started_fmt, t, ctx->id, (int)getpid(), (int)getppid(), ctx->balance);
    log_event(buf);
    if (send_payload_multicast(ctx, STARTED, buf) != 0) return -1;
    if (wait_started_from_others(ctx) != 0) return -1;
    t = get_physical_time();
    snprintf(buf, sizeof(buf), log_received_all_started_fmt, t, ctx->id);
    log_event(buf);
    if (work_until_stop(ctx) != 0) return -1;
    if (done_phase(ctx) != 0) return -1;
    if (send_balance_history(ctx) != 0) return -1;
    return 0;
}

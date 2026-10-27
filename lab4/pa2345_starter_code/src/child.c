#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "child.h"
#include "ipc.h"
#include "lamport.h"
#include "log.h"
#include "pa2345.h"

static int wait_from_other_children(process_context *ctx, MessageType type) {
    Message msg;
    for (local_id from = 1; from < ctx->n; ++from) {
        if (from == ctx->id) continue;
        memset(&msg, 0, sizeof(msg));
        if (receive(ctx, from, &msg) != 0) return -1;
        if (msg.s_header.s_type != type) return -1;
    }
    return 0;
}

static int send_payload_multicast(process_context *ctx, MessageType type, const char *text, timestamp_t t) {
    Message msg;
    memset(&msg, 0, sizeof(msg));
    size_t len = strlen(text);
    msg.s_header.s_magic = MESSAGE_MAGIC;
    msg.s_header.s_payload_len = (uint16_t)len;
    msg.s_header.s_type = type;
    msg.s_header.s_local_time = t;
    memcpy(msg.s_payload, text, len);
    return send_multicast(ctx, &msg);
}

int run_child(process_context *ctx) {
    char buf[MAX_PAYLOAD_LEN];
    timestamp_t t;

    lamport_tick(ctx);
    t = get_lamport_time();
    snprintf(buf, sizeof(buf), log_started_fmt, t, ctx->id, (int)getpid(), (int)getppid(), 0);
    log_event(buf);
    if (send_payload_multicast(ctx, STARTED, buf, t) != 0) return -1;
    if (wait_from_other_children(ctx, STARTED) != 0) return -1;

    snprintf(buf, sizeof(buf), log_received_all_started_fmt, get_lamport_time(), ctx->id);
    log_event(buf);

    int n_iter = ctx->id * 5;
    for (int i = 1; i <= n_iter; ++i) {
        snprintf(buf, sizeof(buf), log_loop_operation_fmt, ctx->id, i, n_iter);
        print(buf);
    }

    lamport_tick(ctx);
    t = get_lamport_time();
    snprintf(buf, sizeof(buf), log_done_fmt, t, ctx->id, 0);
    log_event(buf);
    if (send_payload_multicast(ctx, DONE, buf, t) != 0) return -1;
    if (wait_from_other_children(ctx, DONE) != 0) return -1;

    snprintf(buf, sizeof(buf), log_received_all_done_fmt, get_lamport_time(), ctx->id);
    log_event(buf);
    return 0;
}

#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "child.h"
#include "ipc.h"
#include "lamport.h"
#include "log.h"
#include "pa2345.h"
#include "cs.h"

static int wait_from_other_children(process_context *ctx, MessageType type) {
    if (ctx->n - 2 <= 0) return 0;
    Message msg;
    local_id from;
    int got[MAX_PROCESS_ID + 1] = {0};
    int rc;
    int count = 0;
    if (type == DONE)
        for (from = 1; from < ctx->n; ++from) {
            if (from == ctx->id) continue;
            if (ctx->done_got[from]) {
                got[from] = 1;
                count++;
            }
        }
    while (count < ctx->n - 2) {
        rc = receive_any_nowait_from(ctx, &msg, &from);
        if (rc == 1) continue;
        if (rc < 0) return -1;
        if (from < 1 || from >= ctx->n || from == ctx->id) continue;

        if (msg.s_header.s_type == type) {
            if (!got[from]) {
                got[from] = 1;
                count++;
            }
            if (type == DONE)
                ctx->done_got[from] = 1;
        } else {
            cs_on_peer_msg(ctx, from, &msg);
        }
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

/* it function should be used only for STARTED and DONE messages */
static int announce_and_wait(process_context *ctx, MessageType type, char *buf) {
    lamport_tick(ctx);
    timestamp_t t = get_lamport_time();
    if (type == STARTED)
        snprintf(buf, MAX_PAYLOAD_LEN, log_started_fmt, t, ctx->id, (int)getpid(), (int)getppid(), 0);
    else
        snprintf(buf, MAX_PAYLOAD_LEN, log_done_fmt, t, ctx->id, 0);
    log_event(buf);
    if (send_payload_multicast(ctx, type, buf, t) != 0) return -1;
    return wait_from_other_children(ctx, type);
}

int run_child(process_context *ctx) {
    char buf[MAX_PAYLOAD_LEN];
    if (announce_and_wait(ctx, STARTED, buf) != 0) return -1;
    snprintf(buf, sizeof(buf), log_received_all_started_fmt, get_lamport_time(), ctx->id);
    log_event(buf);
    int n_iter = ctx->id * 5;
    for (int i = 1; i <= n_iter; ++i) {
        if (ctx->use_mutex) {
            if (request_cs(ctx) != 0) return -1;
        }
        snprintf(buf, sizeof(buf), log_loop_operation_fmt, ctx->id, i, n_iter);
        print(buf);
        if (ctx->use_mutex) {
            if (release_cs(ctx) != 0) return -1;
        }
    }
    if (announce_and_wait(ctx, DONE, buf) != 0) return -1;
    snprintf(buf, sizeof(buf), log_received_all_done_fmt, get_lamport_time(), ctx->id);
    log_event(buf);
    return 0;
}

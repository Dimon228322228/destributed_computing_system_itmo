#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "child.h"
#include "ipc.h"
#include "log.h"
#include "pa1.h"

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

static int send_payload_multicast(process_context *ctx, MessageType type, const char *text) {
    Message msg;
    size_t len;

    memset(&msg, 0, sizeof(msg));
    len = strlen(text);

    msg.s_header.s_magic = MESSAGE_MAGIC;
    msg.s_header.s_payload_len = (uint16_t)len;
    msg.s_header.s_type = (int16_t)type;
    msg.s_header.s_local_time = 0;
    memcpy(msg.s_payload, text, len);

    return send_multicast(ctx, &msg);
}

int run_child(process_context *ctx) {
    char buf[MAX_PAYLOAD_LEN];

    snprintf(buf, sizeof(buf), log_started_fmt,
             ctx->id, (int)getpid(), (int)getppid());
    log_event(buf);
    if (send_payload_multicast(ctx, STARTED, buf) != 0) return -1;

    if (wait_from_other_children(ctx, STARTED) != 0) return -1;
    snprintf(buf, sizeof(buf), log_received_all_started_fmt, ctx->id);
    log_event(buf);
    snprintf(buf, sizeof(buf), log_done_fmt, ctx->id);
    log_event(buf);
    if (send_payload_multicast(ctx, DONE, buf) != 0) return -1;
    if (wait_from_other_children(ctx, DONE) != 0) return -1;
    snprintf(buf, sizeof(buf), log_received_all_done_fmt, ctx->id);
    log_event(buf);

    return 0;
}
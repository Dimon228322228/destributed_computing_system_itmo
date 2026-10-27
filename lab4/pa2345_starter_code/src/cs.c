#include <string.h>

#include "cs.h"
#include "lamport.h"

static int cs_less(timestamp_t t1, local_id i1, timestamp_t t2, local_id i2) {
    if (t1 != t2) return t1 < t2;
    return i1 < i2;
}

static void cs_enqueue(process_context *ctx, timestamp_t time, local_id id) {
    int i = ctx->cs_queue_len;
    while (i > 0) {
        cs_queue_entry *p = &ctx->cs_queue[i - 1];
        if (!cs_less(time, id, p->time, p->id)) break;
        ctx->cs_queue[i] = *p;
        i--;
    }
    ctx->cs_queue[i].time = time;
    ctx->cs_queue[i].id = id;
    ctx->cs_queue_len++;
}

static void cs_dequeue_id(process_context *ctx, local_id id) {
    for (int i = 0; i < ctx->cs_queue_len; ++i) {
        if (ctx->cs_queue[i].id != id) continue;
        for (int j = i + 1; j < ctx->cs_queue_len; ++j)
            ctx->cs_queue[j - 1] = ctx->cs_queue[j];
        ctx->cs_queue_len--;
        return;
    }
}

static int cs_is_head(process_context *ctx) {
    return ctx->cs_queue_len > 0 && ctx->cs_queue[0].id == ctx->id;
}

static int cs_all_replied(process_context *ctx) {
    for (local_id i = 1; i < ctx->n; ++i) {
        if (i == ctx->id) continue;
        if (!ctx->cs_replied[i]) return 0;
    }
    return 1;
}

static void cs_mark_reply(process_context *ctx, local_id from) {
    if (from >= 1 && from < ctx->n && from != ctx->id)
        ctx->cs_replied[from] = 1;
}

static int cs_send_empty(process_context *ctx, local_id dst, MessageType type) {
    Message msg;
    memset(&msg, 0, sizeof(msg));
    lamport_tick(ctx);
    msg.s_header.s_magic = MESSAGE_MAGIC;
    msg.s_header.s_type = type;
    msg.s_header.s_local_time = get_lamport_time();
    msg.s_header.s_payload_len = 0;
    return send(ctx, dst, &msg);
}

static int cs_multicast_children(process_context *ctx, MessageType type) {
    for (local_id dst = 1; dst < ctx->n; ++dst) {
        if (dst == ctx->id) continue;
        if (cs_send_empty(ctx, dst, type) != 0) return -1;
    }
    return 0;
}

void cs_handle(process_context *ctx, local_id from, const Message *msg) {
    if (ctx->cs_req_time > 0 && msg->s_header.s_local_time > ctx->cs_req_time)
        cs_mark_reply(ctx, from);
    if (msg->s_header.s_type == CS_REQUEST) {
        cs_enqueue(ctx, msg->s_header.s_local_time, from);
        cs_send_empty(ctx, from, CS_REPLY);
    } else if (msg->s_header.s_type == CS_REPLY) {
        cs_mark_reply(ctx, from);
    } else if (msg->s_header.s_type == CS_RELEASE) {
        cs_dequeue_id(ctx, from);
    }
}

int request_cs(const void *self) {
    process_context *ctx = (process_context *)self;
    if (ctx == NULL) return -1;
    Message msg;
    local_id from;

    memset(ctx->cs_replied, 0, sizeof(ctx->cs_replied));
    lamport_tick(ctx);
    ctx->cs_req_time = get_lamport_time();
    cs_enqueue(ctx, ctx->cs_req_time, ctx->id);

    Message req;
    memset(&req, 0, sizeof(req));
    req.s_header.s_magic = MESSAGE_MAGIC;
    req.s_header.s_type = CS_REQUEST;
    req.s_header.s_local_time = ctx->cs_req_time;
    req.s_header.s_payload_len = 0;
    for (local_id dst = 1; dst < ctx->n; ++dst) {
        if (dst == ctx->id) continue;
        if (send(ctx, dst, &req) != 0) return -1;
    }

    int rc;
    while (!cs_is_head(ctx) || !cs_all_replied(ctx)) {
        rc = receive_any_nowait_from(ctx, &msg, &from);
        if (rc == 1) continue;
        if (rc < 0) return -1;
        if (from == PARENT_ID) continue;
        cs_handle(ctx, from, &msg);
    }
    return 0;
}

int release_cs(const void *self) {
    process_context *ctx = (process_context *)self;
    if (ctx == NULL) return -1;
    Message msg;
    cs_dequeue_id(ctx, ctx->id);
    ctx->cs_req_time = 0;
    lamport_tick(ctx);
    memset(&msg, 0, sizeof(msg));
    msg.s_header.s_magic = MESSAGE_MAGIC;
    msg.s_header.s_type = CS_RELEASE;
    msg.s_header.s_local_time = get_lamport_time();
    msg.s_header.s_payload_len = 0;
    for (local_id dst = 1; dst < ctx->n; ++dst) {
        if (dst == ctx->id) continue;
        if (send(ctx, dst, &msg) != 0) return -1;
    }
    return 0;
}

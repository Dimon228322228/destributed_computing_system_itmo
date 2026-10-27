#include <errno.h>
#include <stddef.h>
#include <string.h>
#include <unistd.h>

#include "pa2_io.h"
#include "ipc.h"
#include "lamport.h"

/* block != 0: inf cycle on EAGAIN (as before)
   block == 0: if EAGAIN/EOF and done==0 → return 1
   return: 0 = OK, 1 = can't (only block == 0), -1 = error 
   it's acceptable to think about both of the read and write functions*/
static int write_full(int fd, const void *buf, size_t len, int block) {
    const char *ptr = buf;
    size_t done = 0;

    while (done < len) {
        ssize_t n = write(fd, ptr + done, len - done);
        if (n > 0) {
            done += (size_t)n;
            continue;
        }
        if (n < 0 && (errno == EAGAIN || errno == EWOULDBLOCK)) {
            if (!block && done == 0) return 1;
            continue;
        }
        return -1;
    }
    return 0;
}

static int read_full(int fd, void *buf, size_t len, int block) {
    char *ptr = buf;
    size_t done = 0;
    while (done < len) {
        ssize_t n = read(fd, ptr + done, len - done);
        if (n > 0) {
            done += (size_t)n;
            continue;
        }
        if (n < 0 && (errno == EAGAIN || errno == EWOULDBLOCK)) {
            if (!block && done == 0) return 1;
            continue;
        }
        if (n == 0) {
            if (done == 0) return block ? -1 : 1;
            return -1;
        }
        return -1;
    }
    return 0;
}

int send(void *self, local_id dst, const Message *msg) {
    process_context *ctx = self;
    if (ctx == NULL || msg == NULL || dst < 0 || dst >= ctx->n || dst == ctx->id) return -1;
    int fd = ctx->pipes[ctx->id][dst].wr;
    if (fd < 0) return -1;

    if (write_full(fd, &msg->s_header, sizeof(MessageHeader), 1) != 0) return -1;
    if (msg->s_header.s_payload_len > 0) 
        if (write_full(fd, msg->s_payload, msg->s_header.s_payload_len, 1) != 0) 
            return -1;
    return 0;
}

int send_multicast(void *self, const Message *msg) {
    process_context *ctx = self;
    if (ctx == NULL || msg == NULL) return -1;
    for (local_id dst = 0; dst < ctx->n; ++dst) {
        if (dst == ctx->id) continue;
        if (send(ctx, dst, msg) != 0) return -1;
    }
    return 0;
}

int receive(void *self, local_id from, Message *msg) {
    process_context *ctx = self;
    if (ctx == NULL || msg == NULL || from < 0 || from >= ctx->n || from == ctx->id) return -1;
    int fd = ctx->pipes[from][ctx->id].rd;
    if (fd < 0) return -1;
    if (read_full(fd, &msg->s_header, sizeof(MessageHeader), 1) != 0) return -1;
    if (msg->s_header.s_payload_len > MAX_PAYLOAD_LEN) return -1;
    if (msg->s_header.s_payload_len > 0) 
        if (read_full(fd, msg->s_payload, msg->s_header.s_payload_len, 1) != 0) 
            return -1;
    lamport_on_receive(ctx, msg->s_header.s_local_time);
    return 0;
}

/* 0 — READ, 1 — STILL EMPTY OR NO LONGER AVAILABLE, -1 — FAIL */
static int try_receive_from(process_context *ctx, local_id from, Message *msg) {
    int fd = ctx->pipes[from][ctx->id].rd;
    if (fd < 0) return 1;
    int rc = read_full(fd, &msg->s_header, sizeof(MessageHeader), 0);
    if (rc != 0) return rc;
    if (msg->s_header.s_payload_len > MAX_PAYLOAD_LEN) return -1;
    if (msg->s_header.s_payload_len > 0) 
        if (read_full(fd, msg->s_payload, msg->s_header.s_payload_len, 1) != 0) 
            return -1;
    lamport_on_receive(ctx, msg->s_header.s_local_time);
    return 0;
}

int send_nowait(void *self, local_id dst, const Message *msg) {
    process_context *ctx = self;
    if (ctx == NULL || msg == NULL) return -1;
    if (dst < 0 || dst >= ctx->n || dst == ctx->id) return -1;
    if (msg->s_header.s_payload_len > MAX_PAYLOAD_LEN) return -1;
    int fd = ctx->pipes[ctx->id][dst].wr;
    if (fd < 0) return -1;
    size_t total = sizeof(MessageHeader) + (size_t)msg->s_header.s_payload_len;
    char buf[sizeof(MessageHeader) + MAX_PAYLOAD_LEN];
    memcpy(buf, &msg->s_header, sizeof(MessageHeader));
    if (msg->s_header.s_payload_len > 0) 
        memcpy(buf + sizeof(MessageHeader), msg->s_payload, msg->s_header.s_payload_len);
    return write_full(fd, buf, total, 0);
}

int receive_any_nowait(void *self, Message *msg) {
    process_context *ctx = self;
    if (ctx == NULL || msg == NULL) return -1;
    for (local_id from = 0; from < ctx->n; ++from) {
        if (from == ctx->id) continue;
        int rc = try_receive_from(ctx, from, msg);
        if (rc == 0) return 0;
        if (rc < 0) return -1;
    }
    return 1;
}

int receive_any_nowait_from(void *self, Message *msg, local_id *from) {
    process_context *ctx = self;
    if (ctx == NULL || msg == NULL || from == NULL) return -1;
    for (local_id f = 0; f < ctx->n; ++f) {
        if (f == ctx->id) continue;
        int rc = try_receive_from(ctx, f, msg);
        if (rc == 0) {
            *from = f;
            return 0;
        }
        if (rc < 0) return -1;
    }
    return 1;
}

int receive_any(void *self, Message *msg) {
    process_context *ctx = self;
    if (ctx == NULL || msg == NULL) return -1;
    for (;;) {
        for (local_id from = 0; from < ctx->n; ++from) {
            if (from == ctx->id) continue;
            int rc = try_receive_from(ctx, from, msg);
            if (rc == 0) return 0;
            if (rc < 0) return -1;
        }
    }
}

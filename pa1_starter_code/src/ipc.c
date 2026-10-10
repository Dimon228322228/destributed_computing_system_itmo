#include <stddef.h>
#include <unistd.h>

#include "pa1_io.h"
#include "ipc.h"

static int write_full(int fd, const void *buf, size_t len) {
    const char *ptr = buf;
    size_t done = 0;

    while (done < len) {
        ssize_t n = write(fd, ptr + done, len - done);
        if (n <= 0) return -1;
        done += (size_t)n;
    }

    return 0;
}

static int read_full(int fd, void *buf, size_t len) {
    char *ptr = buf;
    size_t done = 0;

    while (done < len) {
        ssize_t n = read(fd, ptr + done, len - done);
        if (n <= 0) return -1;
        done += (size_t)n;
    }

    return 0;
}

int send(void *self, local_id dst, const Message *msg) {
    process_context *ctx = self;
    int fd;
    if (ctx == NULL || msg == NULL) return -1;
    if (dst < 0 || dst >= ctx->n || dst == ctx->id) return -1;

    fd = ctx->pipes[ctx->id][dst].wr;
    if (fd < 0) return -1;
    if (write_full(fd, &msg->s_header, sizeof(MessageHeader)) != 0) return -1;

    if (msg->s_header.s_payload_len > 0) 
        if (write_full(fd, msg->s_payload, msg->s_header.s_payload_len) != 0) 
            return -1;

    return 0;
}

int send_multicast(void *self, const Message *msg) {
    process_context *ctx = self;
    local_id dst;
    if (ctx == NULL || msg == NULL) return -1;

    for (dst = 0; dst < ctx->n; ++dst) {
        if (dst == ctx->id) continue;
        if (send(ctx, dst, msg) != 0) return -1;
    }

    return 0;
}

int receive(void *self, local_id from, Message *msg) {
    process_context *ctx = self;
    int fd;
    if (ctx == NULL || msg == NULL) return -1;
    if (from < 0 || from >= ctx->n || from == ctx->id) return -1;

    fd = ctx->pipes[from][ctx->id].rd;
    if (fd < 0) return -1;
    if (read_full(fd, &msg->s_header, sizeof(MessageHeader)) != 0) return -1;
    if (msg->s_header.s_payload_len > MAX_PAYLOAD_LEN) return -1;
    if (msg->s_header.s_payload_len > 0) 
        if (read_full(fd, msg->s_payload, msg->s_header.s_payload_len) != 0) 
            return -1;

    return 0;
}

int receive_any(void *self, Message *msg) {
    process_context *ctx = self;
    local_id from;
    if (ctx == NULL || msg == NULL) return -1;

    for (from = 0; from < ctx->n; ++from) {
        if (from == ctx->id) continue;
        if (receive(ctx, from, msg) == 0) return 0;
    }

    return -1;
}
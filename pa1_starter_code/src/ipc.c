#define _POSIX_C_SOURCE 199309L

#include <stddef.h>
#include <sys/ioctl.h>
#include <time.h>
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

static int bytes_available(int fd) {
    int count = 0;

    if (ioctl(fd, FIONREAD, &count) != 0) return -1;

    return count;
}

static void short_sleep(void) {
    struct timespec delay;

    delay.tv_sec = 0;
    delay.tv_nsec = 1000000;
    nanosleep(&delay, NULL);
}

int send(void *self, local_id dst, const Message *msg) {
    process_context *ctx = self;
    int fd;
    if (ctx == NULL || msg == NULL) return -1;
    if (dst < 0 || dst >= ctx->n || dst == ctx->id) return -1;

    fd = ctx->pipes[ctx->id][dst].wr;
    if (fd < 0) return -1;
    if (msg->s_header.s_payload_len > MAX_PAYLOAD_LEN) return -1;
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
    if (msg->s_header.s_magic != MESSAGE_MAGIC) return -1;
    if (msg->s_header.s_payload_len > MAX_PAYLOAD_LEN) return -1;
    if (msg->s_header.s_payload_len > 0) 
        if (read_full(fd, msg->s_payload, msg->s_header.s_payload_len) != 0) 
            return -1;

    return 0;
}

int receive_any(void *self, Message *msg) {
    process_context *ctx = self;
    local_id from;
    int fd;
    int available;
    if (ctx == NULL || msg == NULL) return -1;

    while (1) {
        for (from = 0; from < ctx->n; ++from) {
            if (from == ctx->id) continue;

            fd = ctx->pipes[from][ctx->id].rd;
            if (fd < 0) continue;

            available = bytes_available(fd);
            if (available < 0) return -1;
            if ((size_t)available >= sizeof(MessageHeader)) {
                return receive(ctx, from, msg);
            }
        }

        short_sleep();
    }
}

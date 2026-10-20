#include <errno.h>
#include <stddef.h>
#include <string.h>
#include <unistd.h>

#include "pa2_io.h"
#include "ipc.h"

static int write_full(int fd, const void *buf, size_t len) {
    const char *ptr = buf;
    size_t done = 0;

    while (done < len) {
        ssize_t n = write(fd, ptr + done, len - done);
        if (n > 0){
            done += (size_t)n;
            continue;
        } 
        if (n < 0 && (errno == EAGAIN || errno == EWOULDBLOCK)) continue;
        return -1;
    }
    return 0;
}

static int read_full(int fd, void *buf, size_t len) {
    char *ptr = buf;
    size_t done = 0;

    while (done < len) {
        ssize_t n = read(fd, ptr + done, len - done);
        if (n > 0){
            done += (size_t)n;
            continue;
        } 
        if (n < 0 && (errno == EAGAIN || errno == EWOULDBLOCK)) continue;
        return -1;
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

/* 0 — прочитали, 1 — пока пусто, -1 — ошибка */
static int try_receive_from(process_context *ctx, local_id from, Message *msg) {
    int fd = ctx->pipes[from][ctx->id].rd;
    char *ptr = (char *)&msg->s_header;
    size_t done = 0;
    size_t need = sizeof(MessageHeader);

    if (fd < 0) return 1;
    while (done < need) {
        ssize_t n = read(fd, ptr + done, need - done);
        if (n > 0) {
            done += (size_t)n;
            continue;
        }
        if (n < 0 && (errno == EAGAIN || errno == EWOULDBLOCK)) {
            if (done == 0) return 1;
            continue;
        }
        /* n == 0: писатель закрыл pipe. Если байт ещё не читали — просто
           пропускаем этот from (как пустой канал), иначе это обрыв сообщения. */
        if (n == 0 && done == 0) return 1;
        return -1;
    }
    if (msg->s_header.s_payload_len > MAX_PAYLOAD_LEN) return -1;
    if (msg->s_header.s_payload_len > 0) 
        if (read_full(fd, msg->s_payload, msg->s_header.s_payload_len) != 0) return -1;
    return 0;
}

int send_nowait(void *self, local_id dst, const Message *msg) {
    process_context *ctx = self;
    int fd;
    size_t total;
    size_t done;
    char buf[sizeof(MessageHeader) + MAX_PAYLOAD_LEN];

    if (ctx == NULL || msg == NULL) return -1;
    if (dst < 0 || dst >= ctx->n || dst == ctx->id) return -1;
    if (msg->s_header.s_payload_len > MAX_PAYLOAD_LEN) return -1;

    fd = ctx->pipes[ctx->id][dst].wr;
    if (fd < 0) return -1;

    total = sizeof(MessageHeader) + (size_t)msg->s_header.s_payload_len;
    memcpy(buf, &msg->s_header, sizeof(MessageHeader));
    if (msg->s_header.s_payload_len > 0) 
        memcpy(buf + sizeof(MessageHeader), msg->s_payload, msg->s_header.s_payload_len);

    done = 0;
    while (done < total) {
        ssize_t n = write(fd, buf + done, total - done);
        if (n > 0) {
            done += (size_t)n;
            continue;
        }
        if (n < 0 && (errno == EAGAIN || errno == EWOULDBLOCK)) {
            if (done == 0) return 1; /* ничего не начали — можно уйти читать */
            continue; /* уже начали — допишем (для DONE обычно не нужно) */
        }
        return -1;
    }
    return 0;
}

int receive_any_nowait(void *self, Message *msg) {
    process_context *ctx = self;
    local_id from;

    if (ctx == NULL || msg == NULL) return -1;

    for (from = 0; from < ctx->n; ++from) {
        int rc;

        if (from == ctx->id) continue;
        rc = try_receive_from(ctx, from, msg);
        if (rc == 0) return 0;
        if (rc < 0) return -1;
    }
    return 1;
}

int receive_any(void *self, Message *msg) {
    process_context *ctx = self;
    local_id from;
    if (ctx == NULL || msg == NULL) return -1;
    for (;;) {
        for (from = 0; from < ctx->n; ++from) {
            int rc;
            if (from == ctx->id) continue;
            rc = try_receive_from(ctx, from, msg);
            if (rc == 0) return 0;
            if (rc < 0) return -1;
        }
    }
}

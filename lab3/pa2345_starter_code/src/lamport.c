#include "banking.h"
#include "lamport.h"

static process_context *g_ctx = NULL;

void lamport_set_context(process_context *ctx) {
    g_ctx = ctx;
}

void lamport_tick(process_context *ctx) {
    ctx->lamport_time++;
}

void lamport_on_receive(process_context *ctx, timestamp_t msg_time) {
    if (ctx->lamport_time < msg_time) ctx->lamport_time = msg_time;
    ctx->lamport_time++;
}

timestamp_t get_lamport_time(void) {
    if (g_ctx == NULL) return 0;
    return g_ctx->lamport_time;
}
#include <string.h>

#include "banking.h"
#include "ipc.h"
#include "pa2_io.h"
#include "lamport.h"

void transfer(void *parent_data, local_id src, local_id dst, balance_t amount) {
    process_context *ctx = parent_data;
    Message msg;
    TransferOrder order;

    memset(&msg, 0, sizeof(msg));
    memset(&order, 0, sizeof(order));

    order.s_src = src;
    order.s_dst = dst;
    order.s_amount = amount;

    lamport_tick(ctx);
    msg.s_header.s_magic = MESSAGE_MAGIC;
    msg.s_header.s_type = TRANSFER;
    msg.s_header.s_local_time = get_lamport_time();
    msg.s_header.s_payload_len = sizeof(TransferOrder);
    memcpy(msg.s_payload, &order, sizeof(order));

    send(ctx, src, &msg);

    /* wait for ACK from receiver */
    memset(&msg, 0, sizeof(msg));
    receive(ctx, dst, &msg);
}

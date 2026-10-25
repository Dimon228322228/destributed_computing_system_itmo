#ifndef LAMPORT_H
#define LAMPORT_H

#include "pa2_io.h"

void lamport_set_context(process_context *ctx);
void lamport_tick(process_context *ctx);
void lamport_on_receive(process_context *ctx, timestamp_t msg_time);

#endif

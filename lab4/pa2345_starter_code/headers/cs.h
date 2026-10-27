#ifndef CS_H
#define CS_H

#include "pa2_io.h"
#include "ipc.h"

void cs_handle(process_context *ctx, local_id from, const Message *msg);

#endif

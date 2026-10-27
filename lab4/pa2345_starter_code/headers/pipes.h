#ifndef PIPES_H
#define PIPES_H

#include "pa2_io.h"

int create_pipes(process_context *ctx);
int log_pipes(const process_context *ctx);
void close_unused_pipes(process_context *ctx);

#endif

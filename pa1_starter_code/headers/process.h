#ifndef PROCESS_H
#define PROCESS_H

#include "pa1_io.h"

typedef enum {
    PROCESS_ROLE_PARENT = 0,
    PROCESS_ROLE_CHILD
} process_role;

/*
 * Создаёт детей, в каждом процессе выставляет ctx->id
 * и закрывает лишние концы pipe.
 * Возвращает роль текущего процесса, либо -1 при ошибке.
 */
int start_processes(process_context *ctx);

#endif

#ifndef HISTORY_H
#define HISTORY_H

#include "banking.h"

/*
 * Дописывает history так, чтобы для каждого t в [0 .. time]
 * был BalanceState. «Дырки» заполняются предыдущим балансом.
 * pending всегда 0 (PA2).
 */
void update_history(BalanceHistory *history, balance_t balance, timestamp_t time);

#endif

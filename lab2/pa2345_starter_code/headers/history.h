#ifndef HISTORY_H
#define HISTORY_H

#include "banking.h"

/*
 * Write up history so that each t in [0 .. time] has BalanceState. 
 * Spaces will be fill up with previous value. Pending = 0 (PA2).
 */
void update_history(BalanceHistory *history, balance_t balance, timestamp_t time);

#endif

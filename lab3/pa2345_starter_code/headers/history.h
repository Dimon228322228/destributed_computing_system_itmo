#ifndef HISTORY_H
#define HISTORY_H

#include "banking.h"

/*
 * Write up history so that each t in [0 .. time] has BalanceState. 
 * Spaces will be fill up with previous value.
 */
 void update_history(BalanceHistory *history, balance_t balance, timestamp_t time, balance_t pending);
 void add_pending(BalanceHistory *history, timestamp_t from, timestamp_t to, balance_t amount);

 #endif

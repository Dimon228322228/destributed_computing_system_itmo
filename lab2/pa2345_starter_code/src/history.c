#include "history.h"

void update_history(BalanceHistory *history, balance_t balance, timestamp_t time){
    balance_t prev;
    timestamp_t t;

    if (history->s_history_len == 0) prev = balance;
    else prev = history->s_history[history->s_history_len - 1].s_balance;

    for (t = history->s_history_len; t < time; ++t) {
        history->s_history[t].s_balance = prev;
        history->s_history[t].s_time = t;
        history->s_history[t].s_balance_pending_in = 0;
    }

    history->s_history[time].s_balance = balance;
    history->s_history[time].s_time = time;
    history->s_history[time].s_balance_pending_in = 0;
    history->s_history_len = (uint8_t)(time + 1);
}
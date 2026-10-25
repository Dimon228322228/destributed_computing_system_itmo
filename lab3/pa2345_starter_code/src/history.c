#include "history.h"

void update_history(BalanceHistory *history, balance_t balance, timestamp_t time, balance_t pending) {
    balance_t prev_bal = history->s_history_len == 0 ? balance
                        : history->s_history[history->s_history_len - 1].s_balance;
    balance_t prev_pend = history->s_history_len == 0 ? 0
                        : history->s_history[history->s_history_len - 1].s_balance_pending_in;

    for (timestamp_t t = history->s_history_len; t < time; ++t) {
        history->s_history[t].s_balance = prev_bal;
        history->s_history[t].s_time = t;
        history->s_history[t].s_balance_pending_in = prev_pend;
    }

    history->s_history[time].s_balance = balance;
    history->s_history[time].s_time = time;
    history->s_history[time].s_balance_pending_in = pending;
    history->s_history_len = (uint8_t)(time + 1);
}

void add_pending(BalanceHistory *history, timestamp_t from, timestamp_t to, balance_t amount) {
    if (from >= to) return;
    if (history->s_history_len < to) {
        balance_t bal = history->s_history_len == 0 ? 0
                        : history->s_history[history->s_history_len - 1].s_balance;
        balance_t pend = history->s_history_len == 0 ? 0
                        : history->s_history[history->s_history_len - 1].s_balance_pending_in;
        update_history(history, bal, (timestamp_t)(to - 1), pend);
    }
    for (timestamp_t t = from; t < to; ++t)
        history->s_history[t].s_balance_pending_in += amount;
}

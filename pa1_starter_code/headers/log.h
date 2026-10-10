#ifndef LOG_H
#define LOG_H

int log_init(void);
void log_close(void);
void log_event(const char *msg);

#endif

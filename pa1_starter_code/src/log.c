#include <stdio.h>

#include "common.h"
#include "log.h"

static FILE *events_file = NULL;

int log_init(void) {
    events_file = fopen(events_log, "w");
    if (events_file == NULL) return -1;
    return 0;
}

void log_close(void) {
    if (events_file != NULL) {
        fclose(events_file);
        events_file = NULL;
    }
}

void log_event(const char *msg) {
    fputs(msg, stdout);
    fflush(stdout);

    if (events_file != NULL) {
        fputs(msg, events_file);
        fflush(events_file);
    }
}

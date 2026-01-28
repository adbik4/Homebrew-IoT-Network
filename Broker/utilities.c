#include "utilities.h"
#include "constants.h"
#include <syslog.h>
#include <stdio.h>

void notice(char* msg) {
    if (RUN_AS_DAEMON) {
        syslog (LOG_NOTICE, "%s", msg);
    } else {
        fprintf(stdout, "%s\n", msg);
    }
}

void error(char* msg) {
    if (RUN_AS_DAEMON) {
        syslog (LOG_ERR, "%s", msg);
    } else {
        fprintf(stderr, "%s\n", msg);
    }
}

void show_stats(int events, int conns) {
    char msg[MAXMSGSIZE];

    sprintf(msg, "Events: %6d | Active connections: %6d", events, conns);
    notice(msg);
}
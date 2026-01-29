#include "broker_util.h"
#include "constants.h"
#include <memory.h>
#include <syslog.h>
#include <stdio.h>

extern Client client_list[MAXCLIENTS];

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
        fprintf(stderr, "\x1B[31m%s\x1B[0m\n", msg); // red
    }
}

void show_stats(int events, int conns) {
    char msg[MAXMSGSIZE];

    sprintf(msg, "Epoll events: %6d | Active connections: %6d", events, conns);
    notice(msg);
}

// finds the index of the client in the client_list
// returns: idx if found, -1 if not.
int client_lookup(int fd) {
    for (int i=0; i < MAXCLIENTS; i++) {
        if (client_list[i].fd == fd) {
            return i;
        }
    }
    return -1; // not found
}

// deletes a client from the client_list
void client_remove(int idx) {
    memset(&(client_list[idx]), 0x00, sizeof(Client));
}
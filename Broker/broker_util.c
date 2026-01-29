#include "broker_util.h"
#include "constants.h"
#include <memory.h>
#include <syslog.h>
#include <stdio.h>
#include <unistd.h>

extern Client client_list[MAXCLIENTS];
extern SensorData last_data[MAXIDCOUNT];

void notice(char* msg) {
    if (RUN_AS_DAEMON) {
        syslog (LOG_NOTICE, "%s", msg);
    } else {
        fprintf(stdout, "%s\n", msg);
    }
}

void info(char* msg) {
    if (RUN_AS_DAEMON) {
        syslog (LOG_INFO, "%s", msg);
    } else {
        fprintf(stdout, "\x1B[33m%s\x1B[0m\n", msg);
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

// sends a notification to a given subscriber
void notify_subscriber(int fd, uint8_t target_id) {
    uint8_t tx_buffer[MAXTXSIZE];

    SensorData data = last_data[target_id];

    data.temperature = htons(data.temperature);
    data.humidity = htons(data.humidity);

    tx_buffer[0] = data.id;
    memcpy(&tx_buffer[1], &data.temperature, 2);
    memcpy(&tx_buffer[3], &data.humidity, 2);

    write(fd, tx_buffer, sizeof(tx_buffer));
}

// notifies all subscribers that have subscribed to a given target id
void notify_all_subscribers(uint8_t target_id) {
    for (size_t i=0; i<MAXCLIENTS; i++) {
        Client* client = &client_list[i];
        if (client->is_subscriber && client->sub == target_id) {
            notify_subscriber(client->fd, target_id);
        }
    }
}
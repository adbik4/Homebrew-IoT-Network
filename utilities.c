#include "utilities.h"
#include "constants.h"
#include "data_structs.h"
#include <memory.h>
#include <syslog.h>
#include <stdio.h>
#include <time.h>

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

// Funkcja PrintMeasurement - wyświetla odebrane dane pomiarowe (ZAMIENIONE: pressure -> humidity)
void PrintMeasurement(const MeasurementData *data) {
    char time_buf[64];
    struct tm *timeinfo;
    
    timeinfo = localtime(&data->timestamp);
    strftime(time_buf, sizeof(time_buf), "%Y-%m-%d %H:%M:%S", timeinfo);
    
    printf("Odebrano pomiar:\n");
    printf("  ID źródła: 0x%02X\n", data->id);
    printf("  Temperatura: %u.%01u°C\n", data->temperature/10, data->temperature%10);
    printf("  Wilgotność: %u.%01u%%\n", data->humidity/10, data->humidity%10);  // ZAMIENIONE: pressure -> humidity
    printf("  Czas pomiaru: %s\n", time_buf);
    printf("----------------------------------------\n");
}
#ifndef DATA_STRUCTS_H
#define DATA_STRUCTS_H
#include "stdint.h"

#define NAME_LEN 64
#define MAX_SUBS 8

struct Client {
    int fd;                     // TCP handle
    char client_id[NAME_LEN];   // human readable name
    uint8_t connected;             // connection state
    uint8_t is_subscriber;         // client type
    uint8_t subs[MAX_SUBS];     // list of subscribed topics
};

#endif
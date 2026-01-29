#ifndef DATA_STRUCTS_H
#define DATA_STRUCTS_H
#include "stdint.h"
#include <netinet/in.h>

#define NAME_LEN 64
#define MAX_SUBS 8

typedef struct {
    int fd;                     // TCP handle
    struct in_addr ip;               // ip
    uint8_t is_subscriber;      // client type
    uint8_t sub;                // subscribed to
} Client;

typedef struct {
    uint8_t id;
    uint16_t temperature;
    uint16_t pressure;
} SensorData;

typedef struct {
    uint8_t special_id;      // 0xFF dla subskrypcji
    uint8_t target_id;       // ID do nasłuchiwania
    uint8_t action;          // 0=start, 1=stop
    uint16_t reserved;
} SubscriptionRequest;

typedef struct {
    uint8_t id;
    time_t timestamp;
    uint16_t temperature;
    uint16_t pressure;
} MeasurementData;

#endif
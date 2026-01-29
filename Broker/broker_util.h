#ifndef UTIL_H
#define UTIL_H
#include "data_structs.h"

void error(char* msg);
void notice(char* msg);
void info(char* msg);

void show_stats(int events, int conns);
int client_lookup(int fd);
void client_remove(int idx);
void notify_subscriber(int fd, uint8_t target_id);
void notify_all_subscribers(uint8_t target_id);

#endif
#ifndef UTIL_H
#define UTIL_H

void error(char* msg);
void notice(char* msg);
void show_stats(int events, int conns);
int client_lookup(int fd);
void client_remove(int idx);

#endif
#ifndef SETUP_H
#define SETUP_H
#include <syslog.h>
#include <sys/types.h>

int daemon_init(const char *pname, int facility, uid_t uid);

int create_udp_discoverer(const int port);
int create_tcp_listener(const int port);

#endif
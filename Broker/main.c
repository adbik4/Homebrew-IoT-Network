#define _GNU_SOURCE

#include "setup.h"
#include "broker_util.h"
#include "database.h"
#include "constants.h"
#include "data_structs.h"

#include <stdlib.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/epoll.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include <unistd.h>
#include <stdio.h>
#include <signal.h>

// globals
static int  epoll_fd;
static int  stat_events, stat_active_conns;
Client client_list[MAXCLIENTS];
char msg[MAXMSGSIZE];

SensorData last_data[MAXIDCOUNT];
FILE *db;

void SignalHandler(int sig);

int main() {
    int discover_fd, listen_fd;
    struct epoll_event events[MAXEVENTS];

    char rx_buffer[BUFSIZE];

    SensorData data;
    SubscriptionRequest request;

    // singal handling registration
    signal(SIGINT, SignalHandler);
    signal(SIGTERM, SignalHandler);

    if (RUN_AS_DAEMON) {
        // transform the server into a daemon
        daemon_init("broker-daemon", LOG_DAEMON, UID);
        notice("broker running (as daemon)");
    } else {
        notice("broker running");
    }
    
    epoll_fd = epoll_create1(0);    // initialise epoll
    if (epoll_fd < 0) {
        error("epoll init failed");
        return -1;
    }

    discover_fd = create_udp_discoverer(DISCOVERY_PORT);     // create a reusable UDP socket
    if (discover_fd > 0) {
        notice("successfully created the discovery port");
    } else {
        return -1;
    }

    listen_fd = create_tcp_listener(LISTEN_PORT);   // create a reusable TCP listen socket
    if (listen_fd > 0) {
        notice("successfully created the listen port");
    } else {
        return -1;
    }
    
    // register with epoll
    struct epoll_event event;
    bzero(&event, sizeof(event));
    event.events = EPOLLIN;
    
    event.data.fd = discover_fd;
    epoll_ctl(epoll_fd, EPOLL_CTL_ADD, discover_fd, &event);

    event.data.fd = listen_fd;
    epoll_ctl(epoll_fd, EPOLL_CTL_ADD, listen_fd, &event);

    // open the database
    if (open_db() < 0) {
        error("the database file couldn't be opened");
    }

    // MAIN LOOP
    while (1) {
        int nready = epoll_wait(epoll_fd, events, MAXEVENTS, -1);
        if (nready < 0) {
            error("epoll_wait failed");
            break;
        }

        stat_events = nready;

        for (int i = 0; i < nready; i++) {
            int fd = events[i].data.fd;

            /* ---------- TCP accept ---------- */
            if (fd == listen_fd) {
                struct sockaddr_in cliaddr;
                socklen_t len = sizeof(cliaddr);

                int client_fd = accept4(listen_fd, (struct sockaddr *)&cliaddr, &len, SOCK_NONBLOCK);
                if (client_fd < 0)
                    continue;

                event.events  = EPOLLIN; // | EPOLLOUT;
                event.data.fd = client_fd;
                epoll_ctl(epoll_fd, EPOLL_CTL_ADD, client_fd, &event);
                
                // add the new client to list
                Client new_client = {
                    .fd = client_fd,
                    .ip = cliaddr.sin_addr,
                    .is_subscriber = 0,
                    .sub = 0
                };
                client_list[stat_active_conns] = new_client;
                stat_active_conns++;

                show_stats(stat_events, stat_active_conns);
                sprintf(msg, "Connection from %s", inet_ntoa(cliaddr.sin_addr));
                info(msg);
                continue;
            }
            /* ---------- UDP echo ---------- */
            if (fd == discover_fd) {
                struct sockaddr_storage peer;
                socklen_t len = sizeof(peer);

                ssize_t n = recvfrom(discover_fd, rx_buffer, sizeof(rx_buffer), 0, (struct sockaddr *)&peer, &len);
                if (n > 0) {
                    // respond for discovery
                    sendto(discover_fd, rx_buffer, n, 0, (struct sockaddr *)&peer, len);
                }
                show_stats(stat_events, stat_active_conns);
                continue;
            }
            
            /* ---------- TCP client ---------- */
            size_t received = 0;
            while (received < BUFSIZE) {
                ssize_t n = read(fd, rx_buffer + received, BUFSIZE - received);
                if (n <= 0) {
                    // disconnect
                    close(fd);

                    stat_active_conns--;
                    show_stats(stat_events, stat_active_conns);

                    // remove the client from the list
                    int cli_idx = client_lookup(fd);
                    sprintf(msg, "%s disconnected", inet_ntoa(client_list[cli_idx].ip));
                    info(msg);
                    client_remove(cli_idx);
                    
                    fd = -1; // invalidate
                    break;
                }
                received += n;
            }

            // if fd is valid
            if (fd >= 0) {  
                // interpret the rx_buffer as SensorData
                data.id = rx_buffer[0]; 
                memcpy(&data.temperature, &rx_buffer[1], 2);
                memcpy(&data.humidity, &rx_buffer[3], 2);

                if (data.id == 0xFF) {  // SUBSCRIBER
                    // reinterpret as SubscriptionRequest
                    request.special_id = rx_buffer[0]; 
                    request.target_id = rx_buffer[1];
                    request.action = rx_buffer[2];

                    // fill in the subscriber data
                    int cli_idx = client_lookup(fd);
                    if (cli_idx < 0) {
                        error("Client index not found");
                    }
                    client_list[cli_idx].is_subscriber = 1;
                    client_list[cli_idx].sub = request.target_id;

                    notify_subscriber(fd, request.target_id);
                } 
                else {  // PUBLISHER
                    data.temperature = ntohs(data.temperature);
                    data.humidity = ntohs(data.humidity);
                    save2db(data);  // save the data
                    sprintf(msg, 
                        "Recieved: ID: 0x%02X | temp: %u.%02u°C | humidity: %u.%u%%",
                        data.id,
                        data.temperature / 10, data.temperature % 10,
                        data.humidity / 10, data.humidity % 10
                    );
                    notice(msg);

                    last_data[data.id] = data;
                    notify_all_subscribers(data.id);
                }
            }
        }
    };

    close(epoll_fd);
    close_db();
    if (RUN_AS_DAEMON) {closelog ();}
    return 0;
}


void SignalHandler(int sig) {
    sprintf(msg, "\nReieved signal %d, exiting program...", sig);
    notice(msg);

    close(epoll_fd);
    close_db();
    if (RUN_AS_DAEMON) {closelog ();}
    exit(0);
}
// PSEUDOKOD SERWERA:
// * przejście w tryb daemon i inicjujemy logi systemowe
// * inicjujemy gniazda
// * ustawiamy działanie epoll()
// * w pętli epoll:
//      - sprawdzamy czy są nowe połączenia
//          jeśli tak: otwieramy nowe połączenie
//      - wyświelaj staty (l. eventów, połączeń)
//      - rozmawiamy z istniejącymi połączeniami
//      - odbieramy pakiet
//          a) to publisher: wyciągamy z niego dane, wpisujemy do pliku, podpisane tematem
//          b) to subscriber: sprawdzamy jakiego tematu zażądał, zapisujemy do listy subscriberów
//          c) to multicast UDP: odpowiadamy na żądanie discovery
//      - czy minał cooldown (żeby nie spamować): 
//          jeśli tak: przeglądamy listę subscriberów, wysyłamy do każdego z nich dane które ich dotyczą (100 ostatnich wpisów)
#define _GNU_SOURCE

#include "setup.h"
#include "utilities.h"
#include "constants.h"

#include <sys/socket.h>
#include <sys/types.h>
#include <sys/epoll.h>
#include <netinet/in.h>
#include <string.h>
#include <unistd.h>
#include <stdio.h>

static int  stat_events;
static int  stat_active_conns;

int main() {
    int discover_fd, listen_fd, epoll_fd;
    struct epoll_event event;
    struct epoll_event events[MAXEVENTS];
    char buffer[MAXBUFSIZE];
    char msg[MAXMSGSIZE];

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
    event.events = EPOLLIN;
    
    event.data.fd = discover_fd;
    epoll_ctl(epoll_fd, EPOLL_CTL_ADD, discover_fd, &event);

    event.data.fd = listen_fd;
    epoll_ctl(epoll_fd, EPOLL_CTL_ADD, listen_fd, &event);

    // MAIN LOOP
    while (1) {
        int nready = epoll_wait(epoll_fd, events, MAXEVENTS, -1);
        if (nready < 0) {
            error("epoll_wait failed");
            break;
        }

        stat_events = nready;

        sprintf(msg, "Events: %6d | Active connections: %6d", stat_events, stat_active_conns);
        notice(msg);
        sleep(100); // nonideal

        for (int i = 0; i < nready; i++) {
            int fd = events[i].data.fd;

            /* ---------- TCP accept ---------- */
            if (fd == listen_fd) {
                struct sockaddr_in cliaddr;
                socklen_t len = sizeof(cliaddr);

                int client_fd = accept4(listen_fd, (struct sockaddr *)&cliaddr, &len, SOCK_NONBLOCK);
                if (client_fd < 0)
                    continue;

                event.events  = EPOLLIN;
                event.data.fd = client_fd;
                epoll_ctl(epoll_fd, EPOLL_CTL_ADD, client_fd, &event);

                stat_active_conns++;
                continue;
            }
            /* ---------- UDP echo ---------- */
            if (fd == discover_fd) {
                struct sockaddr_storage peer;
                socklen_t len = sizeof(peer);

                ssize_t n = recvfrom(discover_fd, buffer, sizeof(buffer), 0, (struct sockaddr *)&peer, &len);
                if (n > 0) {
                    // respond for discovery
                    sendto(discover_fd, buffer, n, 0, (struct sockaddr *)&peer, len);
                }
                continue;
            }
            
            /* ---------- TCP client ---------- */
            ssize_t n = read(fd, buffer, sizeof(buffer));
            if (n <= 0) {
                close(fd);
                stat_active_conns--;
                continue;
            }

            // DO STH

            // echo
            write(fd, buffer, n);
        }
    };

    close(epoll_fd);
    if (RUN_AS_DAEMON) {closelog ();}
    return 0;
}

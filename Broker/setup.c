#include "setup.h"
#include "utilities.h" 
#include "constants.h"
#include <signal.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>

#define	MAXFD	64

int daemon_init(const char *pname, int facility, uid_t uid) {
	int		i;
	pid_t	pid;

	if ( (pid = fork()) < 0)
		return (-1);
	else if (pid)
		exit(0);			/* parent terminates */

	/* child 1 continues... */

	if (setsid() < 0)			/* become session leader */
		return (-1);

	signal(SIGHUP, SIG_IGN);
	if ( (pid = fork()) < 0)
		return (-1);
	else if (pid)
		exit(0);			/* child 1 terminates */

	/* child 2 continues... */

	chdir("/");				/* change working directory - or chroot()*/

	/* close off file descriptors */
	for (i = 0; i < MAXFD; i++){
		close(i);
	}

	/* redirect stdin, stdout, and stderr to /dev/null */
	open("/dev/null", O_RDONLY);
	open("/dev/null", O_RDWR);
	open("/dev/null", O_RDWR);

	openlog(pname, LOG_PID | LOG_NDELAY, facility);
	//openlog (pname, LOG_CONS | LOG_PID | LOG_NDELAY, LOG_LOCAL7);
	
	setuid(uid); /* change user */
	
	return (0);				/* success */
}

int create_udp_discoverer(const int port) {
    int discover_fd;
    char msg[MAXMSGSIZE];
    int reuse;
    unsigned char loop;

    if ((discover_fd = socket(AF_INET, SOCK_DGRAM | SOCK_NONBLOCK, 0)) < 0) {
        snprintf(msg, MAXMSGSIZE, "UDP socket error : %s\n", strerror(errno));
        error(msg);
        return -1;
    }

    // reuse this socket for multiple connections
    reuse = 1;
    if ((setsockopt(discover_fd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse))) < 0) {
		snprintf(msg, MAXMSGSIZE, "UDP setsockopt error : %s\n", strerror(errno));
        error(msg);
        return -1;
	}

    // ignore own messages
    loop = 0;
    setsockopt(discover_fd, IPPROTO_IP, IP_MULTICAST_LOOP, &loop, sizeof(loop));

    struct sockaddr_in addr = {
        .sin_family = AF_INET,
        .sin_port   = htons(port),
        .sin_addr   = { htonl(INADDR_ANY) }
    };

    if (bind(discover_fd, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        snprintf(msg, MAXMSGSIZE, "UDP bind error : %s\n", strerror(errno));
        error(msg);
        return -1;
    }

    // join the multicast group
    struct ip_mreq mreq;
    mreq.imr_multiaddr.s_addr = inet_addr("239.0.0.1");
    mreq.imr_interface.s_addr = htonl(INADDR_ANY);

    if ((setsockopt(discover_fd, IPPROTO_IP, IP_ADD_MEMBERSHIP, &mreq, sizeof(mreq))) < 0) {
		snprintf(msg, MAXMSGSIZE, "UDP mulitcast join error : %s\n", strerror(errno));
        error(msg);
        return -1;
	}

	return discover_fd;
}

int create_tcp_listener(const int port) {
    int listen_fd;
	struct sockaddr_in addr;
    char msg[MAXMSGSIZE];
    int reuse;
	int errno;

    if ((listen_fd = socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK, 0)) < 0) {
        snprintf(msg, MAXMSGSIZE, "TCP listen socket error : %s\n", strerror(errno));
        error(msg);
        return -1;
    }

    // reuse this socket for multiple connections
	reuse = 1;
    if ((setsockopt(listen_fd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse))) < 0) {
		snprintf(msg, MAXMSGSIZE, "TCP setsockopt error : %s\n", strerror(errno));
        error(msg);
        return -1;
	}

	bzero(&addr, sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_port   = htons(port);
	addr.sin_addr.s_addr   = htonl(INADDR_ANY);

    if (bind(listen_fd, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        snprintf(msg, MAXMSGSIZE, "TCP listen bind error : %s\n", strerror(errno));
        error(msg);
        return -1;
    }

	return listen_fd;
}

#ifndef LC_NETWORK_H
#define LC_NETWORK_H

#include "common.h"
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>



#ifndef INVALID_SOCK
#define INVALID_SOCK -1
#endif

int set_sockopt_nonblock(socket_t fd);
int unset_sockopt_nonblock(socket_t fd);
int set_sockopt_nodelay(socket_t fd);

int set_sockopt_rcv_timeout(socket_t fd, int sec);
int set_sockopt_snd_timeout(socket_t fd, int sec);
int set_sockopt_keepalive(socket_t fd, int sec);

int sock_read(socket_t fd, void *buf, size_t bufsize, uint32_t retries);
int sock_write(socket_t fd, struct msghdr *msg, size_t msgsize, uint32_t retries);

socket_t sock_conect_to(ip_addr_t ip, port_t port, int sec);
socket_t sock_create_listen(port_t port, ip_addr_t ip);


#endif

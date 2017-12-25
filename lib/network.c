#ifndef LC_NETWORK_H
#define LC_NETWORK_H

#include "network.h"
#include "common.h"

#include <fcntl.h>
#include <sys/wait.h>
#include <errno.h>
#include <stdlib.h>
#include <sys/time.h>
#include <time.h>
#include <netinet/tcp.h>
#include <netinet/in.h>
#include <unistd.h>
#include <strings.h>
#include <string.h>


int set_sockopt_nonblock(socket_t fd)
{
    int flags;
    flags = fcntl(fd, F_GETFL, 0);
    if(flags < 0)
        return flags;
    flags |= O_NONBLOCK;
    if(fcntl(fd, F_SETFL, &flags) < 0)
        return -1;
    return 0;
}

int unset_sockopt_nonblock(socket_t fd)
{
    int flags;
    flags = fcntl(fd, F_GETFL, 0);
    if(flags < 0)
        return flags;
    flags &= ~O_NONBLOCK;
    if(fcntl(fd, F_SETFL, &flags) < 0)
        return -1;
    return 0;
}

int set_sockopt_nodelay(socket_t fd)
{
    int flag = 1;
    return setsockopt(fd, IPPROTO_TCP, TCP_NODELAY, (char*)&flag, sizeof(flag));
}

int set_sockopt_rcv_timeout(socket_t fd, int sec)
{
    struct timeval  timeout;

    timeout.tv_sec = sec;
    timeout.tv_usec = 0;

    return setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, (char *)&timeout, sizeof(timeout));
}

int set_sockopt_snd_timeout(socket_t fd, int sec)
{
    struct timeval timeout;

    timeout.tv_sec = sec;
    timeout.tv_usec = 0;

    return setsockopt(fd, SOL_SOCKET, SO_SNDTIMEO, (char *)&timeout, sizeof(timeout));
}

/*
 *  I get informations below by <sudo sysctl -a | grep keepalive>
 *
 *      net.ipv4.tcp_keepalive_time = 7200
 *      net.ipv4.tcp_keepalive_probes = 9
 *      net.ipv4.tcp_keepalive_intvl = 75
 *
 *  you can this as you need, just reset the val
 */

int set_sockopt_keepalive(socket_t fd, int timeout)
{
    int val = 1;

    // turn on SO_KEEPALIVE
    if(setsockopt(fd, SOL_SOCKET, SO_KEEPALIVE, &val, sizeof(val)) < 0)
        return -1;

    // change the default action
    val = timeout;
    if(setsockopt(fd, IPPROTO_TCP, TCP_KEEPIDLE, &val, sizeof(val)) < 0)
        return -1;

    val = 5;
    if(setsockopt(fd, IPPROTO_TCP, TCP_KEEPINTVL, &val, sizeof(val)) < 0)
        return -1;

    val = 3; // retries before closing a non-active connection
    if(setsockopt(fd, IPPROTO_TCP, TCP_KEEPCNT, &val, sizeof(val)) < 0)
        return -1;

    return 0;
}

static void forward_iov(struct msghdr *msg, size_t size)
{
    while(msg->msg_iov->iov_len <= size) {
        size -= msg->msg_iov->iov_len;
        msg->msg_iov++;
        msg->msg_iovlen--;
    }

    msg->msg_iov->iov_base = (char *)msg->msg_iov->iov_base + size;
    msg->msg_iov->iov_len -= size;
}

int sock_write(socket_t fd, struct msghdr *msg, size_t len, uint32_t retries)
{
    int ret;

rewrite:
    ret = sendmsg(fd, msg, 0);

    if(ret < 0) {
        if(errno == EINTR)
            goto rewrite;

        if(errno == EAGAIN && retries) {
            retries--;
            goto rewrite;
        }

        return 1;
    }

    len -= ret;
    if(len) {
        forward_iov(msg, ret);
        goto rewrite;
    }

    return 0;
}

int sock_read(socket_t fd, void *buf, size_t len, uint32_t retries)
{
    int     ret;

reread:
    ret = read(fd, buf, len);
    if(ret == 0)
        return 1;

    if(ret < 0) {
        if(errno == EINTR)
            goto reread;

        if(errno == EAGAIN && retries) {
            retries--;
            goto reread;
        }

        return 1;
    }

    len -= ret;
    buf = (char*)buf + ret;
    if(len)
        goto reread;

    return 0;
}

/*
 * set default listen backlog to 20
 */

socket_t sock_create_listen(port_t port, ip_addr_t ip)
{
    int fd;
    int optval;
    struct sockaddr_in servaddr;

    if((fd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
        return -1;

    memset(&servaddr, 0, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(port);
    servaddr.sin_addr.s_addr = ip;

    optval = 1;
    if(setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval)) < 0)
       return -1;

    if((bind(fd, (struct sockaddr *)&servaddr, sizeof(servaddr))) < 0)
        return -1;

    if(listen(fd, 20) < 0)
        return -1;

    return fd;
}


static inline unsigned long time_diff_sec(struct timespec *ts1, struct timespec *ts2)
{
    return ((ts2->tv_sec * 1000000000 + ts2->tv_nsec) -
            (ts1->tv_sec * 1000000000 + ts1->tv_nsec))/1000/1000/1000;
}

socket_t sock_connect_to(ip_addr_t ip, port_t port, int sec)
{
   int fd;
   struct sockaddr_in addr;

   if((fd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
       return -1;

   if(0 != set_sockopt_snd_timeout(fd, sec))
       return -1;

   addr.sin_family = AF_INET;
   addr.sin_port = htons(port);
   addr.sin_addr.s_addr = ip;

   if(connect(fd, (struct sockaddr *)&addr, sizeof(addr)) == -1) {
       close(fd);
       return -1;
   }

   return fd;
}

#endif

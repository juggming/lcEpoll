#include "network.h"
#include "lcepoll.h"

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <sys/uio.h>

lcserver_t *lcs;

typedef struct echo_data {
    uint8_t     c;
} echo_data_t;


int tcpm_send_msg(socket_t s, echo_data_t *hdr, void *data, size_t len)
{
    struct msghdr   msg;
    struct iovec    iov[2];

    memset(&msg, 0, sizeof(msg));

    msg.msg_iov = iov;
    iov[0].iov_base = hdr;
    iov[0].iov_len = sizeof(*hdr);
    msg.msg_iovlen++;

    if(len > 0) {
        msg.msg_iovlen++;
        iov[1].iov_base = data;
        iov[1].iov_len = len;
    }

    return sock_write(s, &msg, sizeof(*hdr) + len, 0);
}

bool accept_new_connection(lcs_conn_t *conn)
{
    printf("new connection: %d %d\n", conn->peer_ip, conn->peer_port);

    set_sockopt_rcv_timeout(conn->s, 5);
    set_sockopt_snd_timeout(conn->s, 5);
    set_sockopt_nodelay(conn->s);

    conn->user_ptr = (void *)0xff;

    return true;
}

bool read_new_request(lcs_conn_t *conn)
{
    uint8_t     d;
    echo_data_t data;

    printf("\nworker id: %d,", conn->idx);
    printf("thread id: %lu, data: ", lcs->slave[conn->idx].tid);

    assert(conn->user_ptr == (void *)0xff);

reread:
    if(sock_read(conn->s, &d, 1, 1) != 0) {
        printf("read error or timeout or peer closed\n");
        return false;
    }

    if(d != '\r' && d != '\n')
        putchar(d);

    data.c = d;
    if(tcpm_send_msg(conn->s, &data, NULL, 0) != 0) {
        printf("send error or timeout\n");
        return false;
    }

    if(d == 'q') {
        printf("\n");
        return false;
    }

    if(d != '\n')
        goto reread;

    printf("\n");
    return true;
}


int main()
{
    lcs_config_t cfg;

    cfg.ip = NULL;
    cfg.max_conns = 200;
    cfg.port = 8888;
    cfg.slave_num = 4;

    lcs = lcserver_create(&cfg);
    assert(lcs);

    lcserver_register_accept(lcs, accept_new_connection);
    lcserver_register_read(lcs, read_new_request);

    assert(lcserver_start(lcs) == 0);

    sleep(30);

    lcserver_stop(lcs);
    lcserver_destroy(lcs);
}

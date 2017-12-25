#ifndef LCEPOLL_H
#define LCEPOLL_H

#include "common.h"
#include "network.h"
#include "pool.h"
#include "event.h"

#include <pthread.h>

struct lcserver;
struct lcs_worker;


typedef struct lcs_config {
    port_t      port;   // listen port
    char *      ip;     // IP to bind
    int         slave_num;
    int         max_conns;
} lcs_config_t;


typedef int lcs_worker_idx;


typedef struct lcs_conn {
    ev_event_t      event;
    socket_t        s;
    ip_addr_t       peer_ip;
    port_t          peer_port;
    lcs_worker_idx  idx;
    struct lcs_worker *worker;
    void *          user_ptr;
} lcs_conn_t;


typedef bool (*lcs_callback_t)(lcs_conn_t *conn);


typedef struct lcs_worker {
    pthread_t       tid;
    int             worker_id;
    struct lcserver *server;
    ev_context_t    *event_context;
} lcs_worker_t;


typedef struct lcserver {
    int32_t         ip;
    port_t          port;
    int             slave_num;
    socket_t        listen_sock;

    lcs_worker_t    master;
    lcs_worker_t    *slave;
    int             next_slave;

    volatile int    stopped;

    /* call when new commection arrives */
    lcs_callback_t  accept;
    lcs_callback_t  read;

    int             max_conns;

    pool_t          *conn_pool;
    pthread_spinlock_t conn_pool_lock;
} lcserver_t;


lcserver_t *lcserver_create(lcs_config_t *cfg);

void lcserver_destroy(lcserver_t *server);

void lcserver_register_accept(lcserver_t *server, lcs_callback_t accept);

void lcserver_register_read(lcserver_t *server, lcs_callback_t read);

int lcserver_start(lcserver_t *server);

void lcserver_stop(lcserver_t *server);

#endif

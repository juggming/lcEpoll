#include "lcepoll.h"

#include <unistd.h>
#include <errno.h>
#include <syslog.h>
#include <assert.h>
#include <sys/select.h>


#define LCS_INVALID_IDX     -1

static inline lcs_conn_t *get_conn(lcserver_t *server)
{
    lcs_conn_t *conn;

    pthread_spin_lock(&server->conn_pool_lock);

    conn = (lcs_conn_t *) pool_alloc_obj(server->conn_pool);

    pthread_spin_unlock(&server->conn_pool_lock);

    return conn;
}

static inline void free_conn(lcserver_t *server, lcs_conn_t *conn)
{
    pthread_spin_lock(&server->conn_pool_lock);

    conn->idx = LCS_INVALID_IDX;
    pool_free_obj(server->conn_pool, conn);

    pthread_spin_unlock(&server->conn_pool_lock);
}

static void conn_read_callback(ev_event_t *event)
{
    lcs_conn_t *conn = (lcs_conn_t *)event;
    lcs_worker_t *worker = conn->worker;
    lcserver_t *server = worker->server;

    if(!server->read(conn)) {
        ev_unregister_event(worker->event_context, event);
        close(conn->s);
        free_conn(server, conn);
    }
}

static void assign_conn_to_worker(lcserver_t *server, lcs_conn_t *conn)
{
    lcs_worker_t *w;
    bool selected_by_user = false;

    if(conn->idx >= 0) {
        selected_by_user = true;
    } else {
        /* default RR policy */
        conn->idx = server->next_slave;
    }

    w = &server->slave[conn->idx];

    conn->event.fd = conn->s;
    conn->event.events = EV_READ_EVENT;
    conn->event.callback = conn_read_callback;

    if(ev_register_event(w->event_context, &conn->event) != 0) {
        syslog(LOG_ERR, "ev_register_event failed: %d", errno);
        close(conn->s);
        free_conn(server, conn);
    }

    /* trans to next worker only on success */
    if(!selected_by_user)
        if(++server->next_slave == server->slave_num)
            server->next_slave = 0;
}

static void *master_worker_thread(void *arg)
{
    struct sockaddr_in cliaddr;
    socket_t sockfd;
    socklen_t socklen;
    int     ret;
    fd_set  set;
    struct timeval select_timeout;
    lcs_conn_t *conn;

    lcs_worker_t *worker = (lcs_worker_t *)arg;
    lcserver_t *server = worker->server;
    socket_t listenfd = server->listen_sock;

    while(!server->stopped) {
        socklen = sizeof(cliaddr);

        // use select first, I will replace it with epoll later
        FD_ZERO(&set);
        FD_SET(listenfd, &set);

        select_timeout.tv_sec = 1;
        select_timeout.tv_usec = 0;
        ret = select(listenfd + 1, &set, NULL, NULL, &select_timeout);
        if(ret == -1) {
            syslog(LOG_ERR, "select return error %d", errno);
            break;
        }
        if(ret == 0)
            continue;

        sockfd = accept(listenfd, (struct sockaddr *)&cliaddr, &socklen);
        if(sockfd != -1) {
            conn = get_conn(server);
            if(!conn) {
                syslog(LOG_ERR,  "up to max connections");
                close(sockfd);
                continue;
            }
            conn->peer_ip = cliaddr.sin_addr.s_addr;
            conn->peer_port = cliaddr.sin_port;
            conn->s = sockfd;
            conn->idx = LCS_INVALID_IDX;

            if(!server->accept(conn)) {
                close(sockfd);
                free_conn(server, conn);
                continue;
            }

            assign_conn_to_worker(server, conn);
        }
    }

    pthread_exit(NULL);
    return NULL;
}

// ??
static void *slave_worker_thread(void *arg)
{
    lcs_worker_t *worker = (lcs_worker_t *)arg;

    if(ev_run(worker->event_context) == -1) {
        syslog(LOG_ERR, "ev_run failed: %d", errno);
        exit(1);
    }

    pthread_exit(NULL);
    return NULL;
}

void lcserver_destroy(lcserver_t *server)
{
    int i;

    if(!server)
        return;

    if(server->slave) {
        for(i = 0; i < server->slave_num; i++) {
            ev_destroy_context(server->slave[i].event_context);
        }

        free(server->slave);
    }

    pthread_spin_destroy(&server->conn_pool_lock);

    if(server->conn_pool)
        pool_destroy(server->conn_pool);
    close(server->listen_sock);
    free(server);
}

lcserver_t *lcserver_create(lcs_config_t *cfg)
{
    lcserver_t *lcs;
    int i;

    lcs = (lcserver_t *)calloc(1, sizeof(lcserver_t));
    if(!lcs)
        return NULL;

    if(cfg->ip) {
        inet_aton(cfg->ip, (struct in_addr *)&lcs->ip);
    } else {
        lcs->ip = INADDR_ANY;
    }


    lcs->port = cfg->port;
    lcs->slave_num = cfg->slave_num;
    lcs->max_conns = cfg->max_conns;
    lcs->conn_pool = pool_create(sizeof(lcs_conn_t), lcs->max_conns);
    if(!lcs->conn_pool)
        goto no_memory;

    if(pthread_spin_init(&lcs->conn_pool_lock, PTHREAD_PROCESS_PRIVATE) != 0) {
        syslog(LOG_ERR, "pthread_spin_init failed: %d", errno);
        lcserver_destroy(lcs);
        return NULL;
    }

    lcs->slave = (lcs_worker_t *)calloc(lcs->slave_num, sizeof(lcs_worker_t));
    if(!lcs->slave)
        goto no_memory;

    for(i = 0; i < lcs->slave_num; i++) {
        lcs->slave[i].event_context = ev_create_context(cfg->max_conns << 1);
        if(!lcs->slave[i].event_context)
            goto no_memory;

        lcs->slave[i].worker_id = i; /* slave varies from 0 to slave_num -1 */
        lcs->slave[i].tid = 0;
        lcs->slave[i].server = lcs;
    }

    lcs->master.event_context = NULL;
    lcs->master.server = lcs;
    lcs->master.tid = 0;
    lcs->master.worker_id = LCS_INVALID_IDX;

    return lcs;

no_memory:
    syslog(LOG_ERR, "no enough memory");
    lcserver_destroy(lcs);
    return NULL;
}

void lcserver_stop(lcserver_t *server)
{
    int     i;

    server->stopped = 1;
    if(server->master.tid > 0)
        pthread_join(server->master.tid, NULL);

    for(i = 0; i < server->slave_num; i++) {
        if(server->slave[i].tid > 0) {
            server->slave[i].event_context->stopped = 1;
            pthread_join(server->slave[i].tid, NULL);
        }
    }
}

int lcserver_start(lcserver_t *server)
{
    int     i;

    server->stopped = 0;
    if(!server->accept || !server->read) {
        syslog(LOG_ERR, "server has not accept and read callback");
        return -1;
    }

    server->listen_sock = sock_create_listen(server->port, server->ip);
    if(server->listen_sock == -1) {
        syslog(LOG_ERR, "sock_create_listen failed: %d", errno);
        return -1;
    }

    if(pthread_create(&server->master.tid, NULL, master_worker_thread, &server->master) != 0) {
        syslog(LOG_ERR, "create master worker failed: %d", errno);
        lcserver_stop(server);
        return -1;
    }

    for(i = 0; i < server->slave_num; i++) {
        if(pthread_create(&server->slave[i].tid, NULL, slave_worker_thread,
                    &server->master) != 0) {
            syslog(LOG_ERR, "create slave worker failed: %d", errno);
            lcserver_stop(server);
            return -1;
        }
    }

    return 0;
}

void lcserver_register_accept(lcserver_t *server, lcs_callback_t accept)
{
    assert(accept);
    server->accept = accept;
}

void lcserver_register_read(lcserver_t *server, lcs_callback_t read)
{
    assert(accept);
    server->read = read;
}

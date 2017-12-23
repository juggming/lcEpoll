#include "event.h"

#include <unistd.h>
#include <time.h>
#include <errno.h>
#include <assert.h>

#define DEFAULT_EPOLL_TIMEOUT   1000    /* 1 sec */

static inline uint64_t get_current_msec()
{
    struct timespec     ts;

    clock_gettime(CLOCK_MONOTONIC, &ts);

    return ts.tv_sec * 1000 + ts.tv_nsec / 1000 / 1000;
}

static inline int64_t run_timers(list_head_t *timer_list)
{
    ev_timer_t  *timer;
    int64_t timeout = DEFAULT_EPOLL_TIMEOUT;

    while(!list_empty(timer_list) &&
            (timer = list_first_entry(timer_list, ev_timer_t, list)))  {
        timeout = timer->abs_msec - get_current_msec();

        if(timeout <= 0) {
            timer->callback(timer);
            list_del(&timer->list);
            timeout = DEFAULT_EPOLL_TIMEOUT;
        }
        else
            break;
    }

    return timeout > DEFAULT_EPOLL_TIMEOUT ? DEFAULT_EPOLL_TIMEOUT : timeout;
}


void ev_destory_context(ev_context_t *c)
{
    close(c->efd);
    free(c);
}


ev_context_t *ev_create_context(int max_events)
{
    size_t size;
    ev_context_t *c;

    assert(max_events > 0);

    size = sizeof(ev_context_t) + max_events * sizeof(struct epoll_event);
    c = (ev_context_t *)calloc(1, size);
    if(!c)
        return NULL;
    c->max_events = max_events;
    c->stopped = 0;
    INIT_LIST_HEAD(&c->timer_list);

    c->efd = epoll_create(max_events);
    if(c->efd == -1) {
        free(c);
        return NULL;
    }

    return c;
}


int ev_run(ev_context_t *c)
{
    int nfds;
    int64_t timeout;
    struct epoll_event *ev;
    ev_event_t *event;
    int i;

    while(!c->stopped) {
        timeout = run_timers(&c->timer_list);

        nfds = epoll_wait(c->efd, c->events, c->max_events, timeout);
        if(nfds == -1) {
            if(errno == EINTR)
                continue;

            return -1;
        }

        for(i = 0; i < nfds; i++) {
            ev = &c->events[i];
            event = (ev_event_t *)(ev->data.ptr);
            event->callback(event);
        }
    }

    return 0;
}

int ev_register_event(ev_context_t *c, ev_event_t *event)
{
    struct epoll_event ev;

    ev.data.ptr = (void *)event;
    ev.events = event->events;

    return epoll_ctl(c->efd, EPOLL_CTL_ADD, event->fd, &ev);
}

void ev_unregister_event(ev_context_t *c, ev_event_t *event)
{
    epoll_ctl(c->efd, EPOLL_CTL_DEL, event->fd, NULL);
}


void ev_init_timer(ev_timer_t *timer, uint64_t msec, ev_timer_callback_t callback)
{
    timer->callback = callback;
    INIT_LIST_HEAD(&timer->list);
    timer->msec = msec;
}

void ev_start_timer(ev_context_t *c, ev_timer_t *timer)
{
    ev_timer_t *t;

    timer->abs_msec = get_current_msec() + timer->msec;
    list_for_each_entry(t, &c->timer_list, list) {
        if(t->abs_msec > timer->abs_msec) {
            __list_add(&timer->list, t->list.prev, &t->list);
            run_timers(&c->timer_list);
            return;
        }
    }
}


void ev_cancel_timer(ev_context_t *c, ev_timer_t *timer)
{
    UNUSED(c);
    list_del(&timer->list);
}

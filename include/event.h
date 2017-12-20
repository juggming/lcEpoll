#ifndef LC_EVENT_H
#define LC_EVENT_H

#include "common.h"
#include "list.h"
#include <sys/epoll.h>


#define EV_TIMER_RESOLUTION     1   // 1 msec

#define EV_READ_EVENT       EPOLLIN
#define EV_WRITE_EVENT      EPOLLOUT

struct ev_event;
struct ev_timer;

typedef void* ev_user_ptr;
typedef void(*ev_event_callback_t)(struct ev_event *event);
typedef void(*ev_timer_callback_t)(struct ev_timer *timer);


typedef struct ev_event {
    int                 fd;
    int                 events;
    ev_event_callback_t callback;
} ev_event_t;


typedef struct ev_timer {
    uint64_t            msec;
    uint64_t            abs_msec;
    ev_timer_callback_t callback;
    ev_user_ptr         data;
    list_head_t         list;
} ev_timer_t;


typedef struct ev_context {
    int                 efd;        //for epoll instance
    volatile int        stopped;
    list_head_t         timer_list;
    int                 max_events; // for epoll
    struct epoll_event  events[0];  // flexible arrays
} ev_context_t;

ev_context_t *ev_create_context(int max_events);

void ev_destory_context(ev_context_t *ptr_context);

int ev_run(ev_context_t *ptr_context);

int ev_register_event(ev_context_t *ptr_context, ev_event_t *event);

void ev_unregister_event(ev_context_t *ptr_context, ev_event_t *event);

void ev_init_timer(ev_timer_t *timer, uint64_t msec, ev_timer_callback_t callback);

void ev_start_timer(ev_context_t *ptr_context, ev_timer_t *timer);

void ev_cancel_timer(ev_context_t *ptr_context, ev_timer_t *timer);

#endif

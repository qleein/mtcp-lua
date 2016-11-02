
/*
 * Copyright (C) Igor Sysoev
 * Copyright (C) Nginx, Inc.
 */


#ifndef _NGX_EVENT_H_INCLUDED_
#define _NGX_EVENT_H_INCLUDED_


#include "rbtree.h"


typedef struct event_s event_t;
typedef void (*event_handler_pt)(event_t *);

struct event_s {
    void        *data;
    int          write;
    int          timedout;
    int          timer_set;

    event_handler_pt     handler;
    rbtree_node_t        timer;
};


#endif /* _TIMER_H_INCLUDED_ */

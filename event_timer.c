
/*
 * Copyright (C) Igor Sysoev
 * Copyright (C) Nginx, Inc.
 */

#include "rbtree.h"
#include "event_timer.h"
#include <stddef.h>


rbtree_t                        event_timer_rbtree;
static rbtree_node_t            event_timer_sentinel;

/*
 * the event timer rbtree may contain the duplicate keys, however,
 * it should not be a problem, because we use the rbtree to find
 * a minimum timer value only
 */

int
event_timer_init()
{
    rbtree_init(&event_timer_rbtree, &event_timer_sentinel,
                rbtree_insert_timer_value);

    return 0;
}


msec_t
event_find_timer(void)
{
    msec_int_t      timer;
    rbtree_node_t  *node, *root, *sentinel;

    if (event_timer_rbtree.root == &event_timer_sentinel) {
        return -1;
    }

    root = event_timer_rbtree.root;
    sentinel = event_timer_rbtree.sentinel;

    node = rbtree_min(root, sentinel);

    //ngx_mutex_unlock(ngx_event_timer_mutex);
    struct timeval tv;
    gettimeofday(&tv, NULL);
    msec_t current_msec = (msec_t) tv.tv_sec * 1000 + tv.tv_usec / 1000;

    timer = (msec_int_t) (node->key - current_msec);

    return (msec_t) (timer > 0 ? timer : 0);
}


void
event_expire_timers(void)
{
    event_t        *ev;
    rbtree_node_t  *node, *root, *sentinel;

    struct timeval tv;
    gettimeofday(&tv, NULL);
    msec_t current_msec = (msec_t) tv.tv_sec * 1000 + tv.tv_usec / 1000;

    sentinel = event_timer_rbtree.sentinel;

    for ( ;; ) {

        //ngx_mutex_lock(ngx_event_timer_mutex);

        root = event_timer_rbtree.root;

        if (root == sentinel) {
            return;
        }

        node = rbtree_min(root, sentinel);

        /* node->key <= ngx_current_time */

        if ((msec_int_t) (node->key - current_msec) <= 0) {
            ev = (event_t *) ((char *) node - offsetof(event_t, timer));

            rbtree_delete(&event_timer_rbtree, &ev->timer);

            //ngx_mutex_unlock(ngx_event_timer_mutex);

#if (NGX_DEBUG)
            ev->timer.left = NULL;
            ev->timer.right = NULL;
            ev->timer.parent = NULL;
#endif

            ev->timer_set = 0;

            ev->timedout = 1;

            ev->handler(ev);

            continue;
        }

        break;
    }

    //ngx_mutex_unlock(ngx_event_timer_mutex);
}

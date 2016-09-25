

#ifndef __MAIN_H__
#define __MAIN_H__

#include "event_timer.h"


#define mtcp_lua_ctx_key  "__mtcp_lua_ctx"

typedef struct mtcp_lua_ctx_s   mtcp_lua_ctx_t;

struct mtcp_lua_ctx_s {
    lua_State           *vm;
    int                 co_ref;
    event_t             ev;
};

mtcp_lua_ctx_t  *mtcp_lua_get_ctx(lua_State *L);
void mtcp_lua_run_thread(mtcp_lua_ctx_t *ctx, int);

int epoll_add_event(int fd, int op, int events, void *ptr);
#endif


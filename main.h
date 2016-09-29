

#ifndef __MAIN_H__
#define __MAIN_H__

#include "rbtree.h"
#include "event.h"
#include <lua.h>
#include <lua.h>
#include <lualib.h>
#include <lauxlib.h>
#include <mtcp_api.h>

#define mtcp_lua_ctx_key            "__mtcp_lua_ctx_t"
#define mtcp_lua_thread_ctx_key     "__mtcp_lua_thread_ctx_t"

typedef struct mtcp_lua_thread_ctx_s   mtcp_lua_thread_ctx_t;

typedef struct {
    int                     core;
    mctx_t                  mctx;
    int                     ep;
    mtcp_lua_thread_ctx_t  *vm_ctx;

    rbtree_t                timer;
    rbtree_node_t           sentinel;

} mtcp_lua_ctx_t;



struct mtcp_lua_thread_ctx_s {
    mtcp_lua_ctx_t      *main;
    lua_State           *vm;
    int                 co_ref;
    event_t             ev;
};


static inline mtcp_lua_thread_ctx_t  *
mtcp_lua_thread_get_ctx(lua_State *L)
{
    mtcp_lua_thread_ctx_t *ctx;
    lua_getglobal(L, mtcp_lua_thread_ctx_key);
    ctx = lua_touserdata(L, -1);
    lua_pop(L, 1);
    return ctx;
}

static inline void
mtcp_lua_thread_set_ctx(lua_State *L, mtcp_lua_thread_ctx_t *ctx)
{
    lua_pushlightuserdata(L, ctx);
    lua_setglobal(L, mtcp_lua_thread_ctx_key);
    return;
}


void mtcp_lua_run_thread(mtcp_lua_thread_ctx_t *ctx, int);


#endif


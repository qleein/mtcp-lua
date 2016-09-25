#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <errno.h>
#include <sys/queue.h>
#include <sys/epoll.h>

#include <lua.h>
#include <lualib.h>
#include <lauxlib.h>

#include "main.h"
#include "event_timer.h"
#include "mtcp_lua_socket_tcp.h"


char mtcp_lua_coroutines_key;




void
mtcp_lua_run_thread(mtcp_lua_ctx_t *ctx, int narg);
void timer_handler(event_t *ev);


lua_State *
mtcp_lua_new_thread(lua_State *L, int *ref)
{
    int base;
    lua_State   *co;

    base = lua_gettop(L);

    lua_pushlightuserdata(L, &mtcp_lua_coroutines_key);
    lua_rawget(L, LUA_REGISTRYINDEX);

    co = lua_newthread(L);
    if (co == NULL) {
        printf("NULLLLL.\n");
    }

    lua_createtable(co, 0, 1);
    lua_pushvalue(co, -1);
    lua_setfield(co, -2, "_G");

    lua_createtable(co, 0, 1);
    lua_pushvalue(co, LUA_GLOBALSINDEX);
    lua_setfield(co, -2, "__index");
    lua_setmetatable(co, -2);

    lua_replace(co, LUA_GLOBALSINDEX);

    *ref = luaL_ref(L, -2);

    if (*ref == LUA_NOREF) {
        lua_settop(L, base);
        return NULL;
    }

    lua_settop(L, base);

    return co;
}

static int
mtcp_lua_thread_spawn(lua_State *L)
{
    int co_ref;
    lua_State *co;

    co = mtcp_lua_new_thread(L, &co_ref);
    
    lua_xmove(L, co, 1);

    mtcp_lua_ctx_t  *ctx = malloc(sizeof(mtcp_lua_ctx_t));
    memset(ctx, 0, sizeof(*ctx));

    ctx->vm = co;
    ctx->co_ref = co_ref;
    ctx->ev.data = ctx;
    ctx->ev.write = 0;
    ctx->ev.timer_set = 0;
    ctx->ev.timedout = 0;
    ctx->ev.handler = timer_handler;
    event_add_timer(&ctx->ev, 1000);

    lua_pushlightuserdata(co, ctx);
    lua_setglobal(co, mtcp_lua_ctx_key);


    mtcp_lua_ctx_t *octx;
    lua_getglobal(L, mtcp_lua_ctx_key);
    octx = lua_touserdata(L, -1);
    lua_pop(L, 1);

    if (octx == NULL) {
        return luaL_error(L, "no mtcp_lua_ctx found");
    }
    event_add_timer(&octx->ev, 2000);
    printf("spawn thread successful and add timer");
    //event_add_timer(&ctx->ev, 0);
    return lua_yield(L, 1);
}


int mtcp_lua_inject_mtcp_thread_api(lua_State *L)
{
    lua_createtable(L, 0, 4);

    lua_pushcfunction(L, mtcp_lua_thread_spawn);
    lua_setfield(L, -2, "spawn");

    lua_setfield(L, -2, "thread");

    return 1;
}

mtcp_lua_ctx_t  *
mtcp_lua_get_ctx(lua_State *L)
{
    mtcp_lua_ctx_t *ctx;
    lua_getglobal(L, mtcp_lua_ctx_key);
    ctx = lua_touserdata(L, -1);
    lua_pop(L, 1);
    return ctx;
}


int mtcp_lua_mtcp_sleep(lua_State *L)
{
    int n = lua_gettop(L);
    if (n != 1) {
        return luaL_error(L, "expect 1 argument, but got %d", n);
    }

    int delay = (int)luaL_checknumber(L, 1) * 1000;
    if (delay < 0) {
        return luaL_error(L, "invalid sleep duration");
    }

    mtcp_lua_ctx_t *ctx;
    lua_getglobal(L, mtcp_lua_ctx_key);
    ctx = lua_touserdata(L, -1);
    lua_pop(L, 1);

    if (ctx == NULL) {
        return luaL_error(L, "no mtcp_lua_ctx found");
    }

    event_add_timer(&ctx->ev, delay);

    return lua_yield(L, 0);
}


int mtcp_lua_inject_mtcp_api(lua_State *L, int flags)
{
    lua_createtable(L, 8, 8);

    lua_pushstring(L, "Hello World!");
    lua_setfield(L, -2, "welcome");

    lua_pushcfunction(L, mtcp_lua_mtcp_sleep);
    lua_setfield(L, -2, "sleep");

    mtcp_lua_inject_mtcp_thread_api(L);
    mtcp_lua_inject_socket_tcp_api(L);

    lua_setglobal(L, "mtcp");

    return 1;
}


static lua_State *lua_init(int);

static lua_State *
lua_init(int flags) {
	lua_State *L = lua_open();
	if (!L) return NULL;

	luaL_openlibs(L);

	if (mtcp_lua_inject_mtcp_api(L, flags) != 1) {
		printf("inject api failed.\n");
		return NULL;
	}

    lua_pushlightuserdata(L, &mtcp_lua_coroutines_key);
    lua_createtable(L, 0, 32);
    lua_rawset(L, LUA_REGISTRYINDEX);

	return L;
}


void
mtcp_lua_run_thread(mtcp_lua_ctx_t *ctx, int narg)
{
    int ret;
    lua_State *L = ctx->vm;
	//ret = lua_pcall(L, 0, LUA_MULTRET, 0);
    //lua_gc(L, LUA_GCCOLLECT, 0);
	ret = lua_resume(L, narg);
    switch (ret) {
    case LUA_YIELD:
        printf("yield.\n");
        return;
    case 0:
        printf("exec file successful.\n");
        break;

    case LUA_ERRRUN:
        printf("runtime error.\n");
        break;
    case LUA_ERRSYNTAX:
        printf("syntax error.\n");
        break;
    
    case LUA_ERRMEM:
        printf("memory allocation error.\n");
        break;

    case LUA_ERRERR:
        printf("error handler error.\n");
        break;

    default:
        printf("unknown error.\n");
        break;
    }


	if (ret != 0) {
		printf("execute lua script failed:%d.\n", ret);
		printf("res:%s.\n", luaL_checkstring(L, -1));
	}

	//lua_close(L);

	//printf("Come here.\n");
	return;
}


void timer_handler(event_t *ev)
{
    //printf("time reach.\n");

    mtcp_lua_ctx_t *ctx = ev->data;

    mtcp_lua_run_thread(ctx, 0);

    return;
}


void process_events_and_timers(void)
{
    printf("process_events_and_timers called.\n");

    event_expire_timers();
    return;
}


int
main_thread_init(void)
{

	lua_State *L = lua_init(0);
	if (!L) return 1;

	int ret = luaL_loadfile(L, "test.lua");
	if (ret != 0) {
		printf("execute lua file failed:%d.\n",ret);
		printf("res:%s.\n", luaL_checkstring(L, 1));
        exit(1);
	}

    mtcp_lua_ctx_t *ctx = malloc(sizeof(mtcp_lua_ctx_t));
    ctx->vm = L;

    ctx->ev.data = ctx;
    ctx->ev.write = 0;
    ctx->ev.timer_set = 0;
    ctx->ev.timedout = 0;
    ctx->ev.handler = timer_handler;
    event_add_timer(&ctx->ev, 0);

    lua_pushlightuserdata(L, ctx);
    lua_setglobal(L, mtcp_lua_ctx_key);

    return 0;
}


int epoll_fd = -1;

int epoll_add_event(int fd, int op, int events, void *ptr)
{
    int rc;
    struct epoll_event ev;
    ev.events = events | EPOLLET;
    ev.data.ptr = ptr;
    rc = epoll_ctl(epoll_fd, op, fd, &ev);
    if (rc < 0) {
        printf("epoll ctl failed.\n");
        return -1;
    }
    printf("fd:%d, op:%d, ptr:%lu.\n", fd, op, ptr);
    return 0;
}

int
main(int argc, char *argv[])
{

    epoll_fd = epoll_create1(0);
    if (epoll_fd < 0) {
        printf("epoll create failed.\n");
        exit(1);
    }



    event_timer_init();

    main_thread_init();

#define MAX_EPOLL_SIZE  1000
    int nfds;
    struct epoll_event      events[MAX_EPOLL_SIZE];
    while (1) {
        nfds = epoll_wait(epoll_fd, events, MAX_EPOLL_SIZE, 500);
        if (nfds < 0) {
            if (errno != EINTR) {
                printf("epoll wait failed.\n");
                break;
            }
            continue;
        }

        int i;
        for (i = 0; i < nfds; i++) {
            event_t *ev = events[i].data.ptr;
        printf("ev:%lu\n", ev);
            if (ev->handler) {
                printf("exec event handler.\n");
                ev->handler(ev);
            } else printf("no handler defined for event.\n");
            //rc = process_event(events[i].data.ptr);
        }


        event_expire_timers();
        sleep(1);
        printf("sleep for 1s in main cycle.\n");
    }

    return 0;
}



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

#include <mtcp_api.h>
#include <mtcp_epoll.h>
#include <debug.h>



char mtcp_lua_coroutines_key;
char mtcp_lua_mtcp_context_key;

char *global_lua_script = NULL;

#define MAX_CORE_LIMIT          16
#define MAX_EPOLL_SIZE          1024

static mtcp_lua_ctx_t   global_context[MAX_CORE_LIMIT];
static pthread_t        app_thread[MAX_CORE_LIMIT];



void
mtcp_lua_run_thread(mtcp_lua_thread_ctx_t *ctx, int narg);
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
    mtcp_lua_thread_ctx_t *octx;
    lua_getglobal(L, mtcp_lua_ctx_key);
    octx = lua_touserdata(L, -1);
    lua_pop(L, 1);

    if (octx == NULL) {
        return luaL_error(L, "no mtcp_lua_ctx found");
    }

    int co_ref;
    lua_State *co;

    co = mtcp_lua_new_thread(L, &co_ref);
    lua_xmove(L, co, 1);

    mtcp_lua_thread_ctx_t  *ctx = malloc(sizeof(mtcp_lua_thread_ctx_t));
    memset(ctx, 0, sizeof(*ctx));

    ctx->vm = co;
    ctx->main = octx->main;
    ctx->co_ref = co_ref;
    ctx->ev.data = ctx;
    ctx->ev.write = 0;
    ctx->ev.timer_set = 0;
    ctx->ev.timedout = 0;
    ctx->ev.handler = NULL;

    lua_pushlightuserdata(co, ctx);
    lua_setglobal(co, mtcp_lua_ctx_key);

    mtcp_lua_run_thread(ctx, 0);
    
    printf("spawn thread successful");
    lua_pushboolean(L, 1);
    return 1;
}


int mtcp_lua_inject_mtcp_thread_api(lua_State *L)
{
    lua_createtable(L, 0, 4);

    lua_pushcfunction(L, mtcp_lua_thread_spawn);
    lua_setfield(L, -2, "spawn");

    lua_setfield(L, -2, "thread");

    return 1;
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

    mtcp_lua_thread_ctx_t *ctx;
    ctx = mtcp_lua_thread_get_ctx(L);
    if (ctx == NULL) {
        return luaL_error(L, "no mtcp_lua_ctx found");
    }

    event_add_timer(ctx->main, &ctx->ev, delay);

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
mtcp_lua_run_thread(mtcp_lua_thread_ctx_t *ctx, int narg)
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

    mtcp_lua_thread_ctx_t *ctx = ev->data;

    mtcp_lua_run_thread(ctx, 0);

    return;
}


void process_events_and_timers(void)
{
    printf("process_events_and_timers called.\n");

    event_expire_timers();
    return;
}


mtcp_lua_thread_ctx_t *
mtcp_lua_vm_init(mtcp_lua_ctx_t *ctx)
{

	lua_State *L = lua_init(0);
	if (!L) return NULL;

	int ret = luaL_loadfile(L, global_lua_script);
	if (ret != 0) {
		printf("execute lua file failed:%d.\n",ret);
		printf("res:%s.\n", luaL_checkstring(L, 1));
        exit(1);
	}

    mtcp_lua_thread_ctx_t *lctx = malloc(sizeof(mtcp_lua_thread_ctx_t));
    lctx->vm = L;
    lctx->main = ctx;

    lctx->ev.data = ctx;
    lctx->ev.write = 0;
    lctx->ev.timer_set = 0;
    lctx->ev.timedout = 0;
    lctx->ev.handler = timer_handler;
    event_add_timer(ctx, &lctx->ev, 0);

    lua_pushlightuserdata(L, lctx);
    lua_setglobal(L, mtcp_lua_thread_ctx_key);

    return lctx;
}


/*  
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
*/


void *
thread_entry(void *arg)
{
    int core = *(int *)arg;

    if (core < 0 || core > MAX_CORE_LIMIT) {
        TRACE_ERROR("Invalid core number".\n);
        return NULL;
    }

    mtcp_lua_ctx_t *ctx;
    mctx_t  mctx;

    mctx = mtcp_create_context(core);
    if (!ctx->mctx) {
        TRACE_ERROR("Failed to create mtcp context.\n");
        return NULL;
    }

    ctx = &global_context[core];
    ctx->mctx = mctx;
    ctx->core = core;

    mtcp_core_affinitize(core);

    int maxevents = 1024;
    //mtcp_init_rss(mctx, saddr, IP_RANGE, daddr, dport);
    int ep = mtcp_epoll_create(mctx, maxevents);
    if (ep < 0) {
        TRACE_ERROR("Failed to create epoll struct.\n");
        exit(EXIT_FAILURE);
    }
    ctx->ep = ep;

    int max_events = MAX_EPOLL_SIZE;
    struct mtcp_epoll_event     *events;
    events = (struct mtcp_epoll_event *)calloc(maxevents, sizeof(struct mtcp_epoll_event));
    if (events == NULL) {
        TRACE_ERROR("Failed to create event struct.\n");
        exit(EXIT_FAILURE);
    }

    ctx->vm_ctx = mtcp_lua_vm_init(ctx);
    if (ctx->vm_ctx == NULL) {
        TRACE_ERROR("Failed to init lua.\n");
        exit(EXIT_FAILURE);
    }

    event_timer_init(ctx);

    int nevents;
    for (;;) {
        nevents = mtcp_epoll_wait(mctx, ep, events, maxevents, 500);
        if (nevents < 0) {
            if (errno != EINTR) {
                TRACE_ERROR("mtcp_epoll_wait failed, ret:%d\n", nevents);
            }
        }

        int i;
        for (i = 0; i < nevents; i++) {
            event_t *ev = events[i].data.ptr;
            if (!ev->handler) {
                TRACE_ERROR("No handler defined for current event.\n");
                continue;
            }
            
            ev->handler(ev);
        }

        event_expire_timers();
        printf("a main cycle end.\n");
    }

    return 0;
}


int main(int argc, char **argv)
{
    struct mtcp_conf    mcfg;

    if (argc != 2) {
        TRACE_CONFIG("Usage: %s lua_script\n", argv[0]);
        return -1;
    }

    global_lua_script = argv[1];

    int num_cores = GetNumCPUs();
    int core_limit = num_cores < MAX_CORE_LIMIT ? num_cores : MAX_CORE_LIMIT;
    int ret = mtcp_init("mtcp_lua.conf");
    if (ret) {
        TRACE_ERROR("Failed to initialize mtcp.\n");
        exit(EXIT_FAILURE);
    }

    mtcp_getconf(&mcfg);
    if (mcfg.num_cores > core_limit) {
        mcfg.num_cores = core_limit;
    }
    num_cores = mcfg.num_cores;

    mtcp_setconf(&mcfg);

    int i;

    int cores[MAX_CORE_LIMIT];
    for (i = 0; i < num_cores; i++) {
        cores[i] = i;
        if (pthread_create(&app_thread[i], NULL, thread_entry,
                           (void *)&cores[i]))
        {
            TRACE_ERROR("Failed to create thread.\n");
            exit(-1);
        }
    }

    for (i = 0; i < num_cores; i++) {
        pthread_join(app_thread[i], NULL);
        TRACE_INFO("thread %d joined.\n", i);
    }

    mtcp_destroy();
    return 0;
}

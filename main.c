#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <errno.h>
#include <sys/queue.h>

#include <lua.h>
#include <lualib.h>
#include <lauxlib.h>

#include "event_timer.h"


#define mtcp_lua_ctx_key  "__mtcp_lua_ctx"

typedef struct mtcp_lua_ctx_s   mtcp_lua_ctx_t;

struct mtcp_lua_ctx_s {
    lua_State           *vm;

    event_t             ev;
};


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

	return L;
}


void
mtcp_lua_run_thread(mtcp_lua_ctx_t *ctx)
{
    int ret;
    lua_State *L = ctx->vm;
	//ret = lua_pcall(L, 0, LUA_MULTRET, 0);
	ret = lua_resume(L, 0);
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

	lua_close(L);

	printf("Come here.\n");

	return;
}


void timer_handler(event_t *ev)
{
    printf("time reach.\n");

    mtcp_lua_ctx_t *ctx = ev->data;

    mtcp_lua_run_thread(ctx);

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



int
main(int argc, char *argv[])
{

    event_timer_init();

    main_thread_init();

    while (1) {
        event_expire_timers();
        sleep(1);
        printf("sleep for 1s in main cycle.\n");
    }

    return 0;
}



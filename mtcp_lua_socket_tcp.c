#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <errno.h>
#include <sys/queue.h>

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include <lua.h>
#include <lualib.h>
#include <lauxlib.h>

#include "event_timer.h"

static const char *libname = "mtcp_lua_socket_tcp";

typedef struct {
    int         fd;
    int         connected;
    int         busy;
    lua_State   *vm;
    int         timeout;

} mtcp_lua_socket_tcp_ctx_t;


static int mtcp_lua_socket_tcp(lua_State *L)
{
    mtcp_lua_socket_tcp_ctx_t *sctx;

    sctx = (mtcp_lua_socket_tcp_ctx_t *)lua_newuserdata(L, sizeof(*sctx));
    sctx->fd = -1;
    sctx->busy = 0;
    sctx->vm = L;
    sctx->timeout = 0;


    luaL_getmetatable(L, libname);
    lua_setmetatable(L, -2);

    return 1;
}

static int mtcp_lua_socket_tcp_connect(lua_State *L)
{
    mtcp_lua_socket_tcp_ctx_t *sctx;
    sctx = (mtcp_lua_socket_tcp_ctx_t *)lua_touserdata(L, 1);
    if (sctx->fd != 1) {
        close(sctx->fd);
    }

    int fd;
    fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd < 0) {
        lua_pushnil(L);
        lua_pushliteral(L, "create socket failed");
        return 2;
    }

    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = inet_addr("127.0.0.1");
    addr.sin_port = htons(9119);

    int ret;
    ret = connect(fd, (struct sockaddr *)&addr, sizeof(addr));
    if (ret < 0) {
        close(fd);
        lua_pushnil(L);
        lua_pushliteral(L, "connect failed");
        return 2;
    }

    sctx->fd = fd;
    sctx->connected = 1;

    lua_pushboolean(L, 1);
    return 1;
}


static int mtcp_lua_socket_tcp_close(lua_State *L)
{
    mtcp_lua_socket_tcp_ctx_t *sctx;
    sctx = (mtcp_lua_socket_tcp_ctx_t *)lua_touserdata(L, 1);
    if (sctx->fd != 1) {
        close(sctx->fd);
    }

    lua_pushboolean(L, 1);
    return 1;
}


static int mtcp_lua_socket_tcp_gc(lua_State *L) {
    mtcp_lua_socket_tcp_ctx_t *sctx;
    sctx = (mtcp_lua_socket_tcp_ctx_t *)lua_touserdata(L, 1);
    if (sctx->fd != 1) {
        close(sctx->fd);
    }
    printf("gcc called.\n");
    lua_pushboolean(L, 1);
    return 1;
}

int mtcp_lua_inject_socket_tcp_api(lua_State *L)
{
    lua_createtable(L, 0, 4);

    lua_pushcfunction(L, mtcp_lua_socket_tcp);
    lua_setfield(L, -2, "tcp");

    lua_setfield(L, -2, "socket");

    /*  create metatable */
    luaL_newmetatable(L, libname);
    lua_pushcfunction(L, mtcp_lua_socket_tcp_connect);
    lua_setfield(L, -2, "connect");

    lua_pushcfunction(L, mtcp_lua_socket_tcp_close);
    lua_setfield(L, -2, "close");

    lua_pushcfunction(L, mtcp_lua_socket_tcp_gc);
    lua_setfield(L, -2, "__gc");

    lua_pushvalue(L, -1);
    lua_setfield(L, -2, "__index");
    lua_pop(L, 1);

    return 1;
}

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <errno.h>
#include <sys/queue.h>

#include <fcntl.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/epoll.h>

#include <lua.h>
#include <lualib.h>
#include <lauxlib.h>

#include "main.h"
#include "event_timer.h"

static const char *libname = "mtcp_lua_socket_tcp";

typedef struct {
    int         fd;
    int         connected;
    int         busy;
    lua_State   *vm;
    int         connect_timeout;
    int         read_timeout;
    int         write_timeout;

} mtcp_lua_socket_tcp_ctx_t;


static int mtcp_lua_socket_tcp(lua_State *L)
{
    int n = lua_gettop(L);
    if (n != 0) {
        return luaL_error(L, "expect 0 arguments, but got %d", n);
    }


    mtcp_lua_socket_tcp_ctx_t *sctx;

    sctx = (mtcp_lua_socket_tcp_ctx_t *)lua_newuserdata(L, sizeof(*sctx));
    sctx->fd = -1;
    sctx->busy = 0;
    sctx->vm = L;
    sctx->connect_timeout = 6;
    sctx->read_timeout = 6;
    sctx->write_timeout = 6;

    luaL_getmetatable(L, libname);
    lua_setmetatable(L, -2);

    return 1;
}


void mtcp_lua_socket_tcp_connect_handler(event_t *ev)
{

    mtcp_lua_socket_tcp_ctx_t *sctx = ev->data;
    mtcp_lua_ctx_t *ctx = mtcp_lua_get_ctx(sctx->vm);
    lua_State *L = sctx->vm;

    if (ev->timedout) {
        epoll_add_event(sctx->fd, EPOLL_CTL_DEL, 0, ev);
        lua_pushnil(L);
        lua_pushliteral(L, "connect timeout");
        return mtcp_lua_run_thread(ctx, 2);
    } else {
        event_del_timer(ev);
    }

    int err = 0;
    socklen_t len = sizeof(int);
    if (getsockopt(sctx->fd, SOL_SOCKET, SO_ERROR, (void*)&err, &len) == -1) {
        err = errno;
    }

    if (err) {
        printf("connect failed, errno:%d. err:%s.\n", err, strerror(err));
        return;
    }

    sctx->connected = 1;

    lua_pushboolean(L, 1);
    lua_pushliteral(L, "connect successful");
    return mtcp_lua_run_thread(ctx, 2);
}


static int mtcp_lua_socket_tcp_connect(lua_State *L)
{
    int n = lua_gettop(L);
    if (n != 3) {
        return luaL_error(L, "expect 3 arguments, but got %d", n);
    }

    mtcp_lua_socket_tcp_ctx_t *sctx;
    sctx = (mtcp_lua_socket_tcp_ctx_t *)lua_touserdata(L, 1);
    if (sctx->fd != -1) {
        printf("Socket conneced. close current.\n");
        close(sctx->fd);
    }

    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;

    const char *host = luaL_checkstring(L, 2);
    int ret = inet_pton(AF_INET, host, &addr.sin_addr.s_addr);
    if (ret != 1) {
        printf("invalid ip address.\n");
        lua_pushnil(L);
        lua_pushliteral(L, "invalid ip address");
        return 2;
    }

    int port = luaL_checkint(L, 3);
    if (port < 0 || port > 65535) {
        return luaL_error(L, "invalid port");
    }
    addr.sin_port = htons(port);

    int fd;
    fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd < 0) {
        lua_pushnil(L);
        lua_pushliteral(L, "create socket failed");
        return 2;
    }

    //set socket unblocking
    if(fcntl(fd, F_SETFL, fcntl(fd, F_GETFL) | O_NONBLOCK) < 0) {
        lua_pushnil(L);
        lua_pushliteral(L, "set noblock failed");
        return 2;
    }

    ret = connect(fd, (struct sockaddr *)&addr, sizeof(addr));
    if (ret == 0) {
        sctx->fd = fd;
        sctx->connected = 1;
        lua_pushboolean(L, 1);
        return 1;
    }

    int err;
    err = errno;
    if (err != EINPROGRESS && err != EAGAIN) {
        printf("ret:%d, errno:%d, err:%s.\n", ret, errno, strerror(errno));
        lua_pushnil(L);
        lua_pushliteral(L, "connct failed.");
        return 2;
    }

    sctx->fd = fd;
    sctx->vm = L;

    mtcp_lua_ctx_t *ctx = mtcp_lua_get_ctx(L);
    event_t *ev = &ctx->ev;
    ev->data = sctx;
    ev->timedout = 0;
    ev->handler = mtcp_lua_socket_tcp_connect_handler;

    event_add_timer(ev, sctx->connect_timeout);
    epoll_add_event(sctx->fd, EPOLL_CTL_ADD, EPOLLOUT, ev);

    return lua_yield(L, 0);
}


static int mtcp_lua_socket_tcp_send(lua_State *L)
{
    int n = lua_gettop(L);
    if (n != 2) {
        return luaL_error(L, "expect 2 arguments, but got %d", n);
    }

    mtcp_lua_socket_tcp_ctx_t *sctx;
    sctx = (mtcp_lua_socket_tcp_ctx_t *)lua_touserdata(L, 1);
    if (sctx->fd == -1 || sctx->connected != 1) {
        return luaL_error(L, "socket not connected.");
    }

    size_t len;
    const char *data = luaL_checklstring(L, 2, &len);
    if (write(sctx->fd, data, len) != len) {
        lua_pushnil(L);
        lua_pushliteral(L, "write socket failed");
        return 2;
    }

    lua_pushboolean(L, 1);
    return 1;
}


void mtcp_lua_socket_tcp_recv_handler(event_t *ev)
{
    printf("Come here.\n");
    mtcp_lua_socket_tcp_ctx_t *sctx = ev->data;
    lua_State *L = sctx->vm;
    mtcp_lua_ctx_t *ctx = mtcp_lua_get_ctx(L);
    if (ev->timedout) {
        epoll_add_event(sctx->fd, EPOLL_CTL_DEL, 0, 0);
        lua_pushnil(L);
        lua_pushliteral(L, "recv timedout");
        return mtcp_lua_run_thread(ctx, 2);
    } else {
        event_del_timer(ev);
    }



    int n;
    char buffer[1024];

    n = read(sctx->fd, buffer, 1024);

    if (n > 0) {
        lua_pushlstring(L, buffer, n);
        return mtcp_lua_run_thread(ctx, 1);
    }

    if (n == 0) {
        lua_pushnil(L);
        lua_pushliteral(L, "connection closed");
        return mtcp_lua_run_thread(ctx, 2);
    }
    if (n < 0) {
        lua_pushnil(L);
        lua_pushliteral(L, "recv failed");
        return mtcp_lua_run_thread(ctx, 2);
    }
    return;
}


static int
mtcp_lua_socket_tcp_settimeout(lua_State *L)
{
    int n = lua_gettop(L);
    if (n != 1) {
        return luaL_error(L, "expect 2 arguments, but got %d", n);
    }

    mtcp_lua_socket_tcp_ctx_t *sctx;
    sctx = (mtcp_lua_socket_tcp_ctx_t *)lua_touserdata(L, 1);

    int timeout = luaL_checkint(L, 2);
    if (timeout <= 0 || timeout > 600) {
        return luaL_error(L, "exceed limit.");
    }
    sctx->connect_timeout = timeout;
    sctx->read_timeout = timeout;
    sctx->write_timeout = timeout;

    lua_pushboolean(L, 1);
    return 1;
}

static int mtcp_lua_socket_tcp_recv(lua_State *L)
{
    int n = lua_gettop(L);
    if (n != 1) {
        return luaL_error(L, "expect 1 arguments, but got %d", n);
    }

    mtcp_lua_socket_tcp_ctx_t *sctx;
    sctx = (mtcp_lua_socket_tcp_ctx_t *)lua_touserdata(L, 1);
    if (sctx->fd == -1 || sctx->connected != 1) {
        return luaL_error(L, "socket not connected.");
    }

    char buffer[1024];
    n = read(sctx->fd, buffer, 1024);
    if (n > 0) {
        lua_pushlstring(L, buffer, n);
        return 1;
    }

    if (errno != EAGAIN) {
        lua_pushnil(L);
        lua_pushliteral(L, "read socket failed");
        return 2;
    }

    sctx->vm = L;
    //sctx->busy = 1;
    
    mtcp_lua_ctx_t *ctx = mtcp_lua_get_ctx(L);
    event_t *ev = &ctx->ev;
    ev->data = sctx;
    ev->handler = mtcp_lua_socket_tcp_recv_handler;

    event_add_timer(ev, 2);

    epoll_add_event(sctx->fd, EPOLL_CTL_ADD, EPOLLIN, ev);

    printf("recv yield.\n");
    return lua_yield(L, 0);
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

    lua_pushcfunction(L, mtcp_lua_socket_tcp_settimeout);
    lua_setfield(L, -2, "settimeout");

    lua_pushcfunction(L, mtcp_lua_socket_tcp_send);
    lua_setfield(L, -2, "send");

    lua_pushcfunction(L, mtcp_lua_socket_tcp_recv);
    lua_setfield(L, -2, "recv");

    lua_pushcfunction(L, mtcp_lua_socket_tcp_close);
    lua_setfield(L, -2, "close");

    lua_pushcfunction(L, mtcp_lua_socket_tcp_gc);
    lua_setfield(L, -2, "__gc");

    lua_pushvalue(L, -1);
    lua_setfield(L, -2, "__index");
    lua_pop(L, 1);

    return 1;
}

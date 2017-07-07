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

#include <mtcp_api.h>
#include <mtcp_epoll.h>
#include <debug.h>

static const char *libname = "mtcp_lua_socket_tcp";

typedef struct {
    int         fd;
    int         connected;
    int         busy;
    int         active;
    lua_State   *vm;
    mtcp_lua_thread_ctx_t *vm_ctx;
    int         connect_timeout;
    int         read_timeout;
    int         write_timeout;

    struct sockaddr     sockaddr;
    socklen_t           socklen;


} mtcp_lua_socket_tcp_ctx_t;


static int
mtcp_lua_socket_tcp_close_helper(mtcp_lua_thread_ctx_t *ctx,
    mtcp_lua_socket_tcp_ctx_t *sctx);


static inline int
mtcp_lua_epoll_ctl(mtcp_lua_socket_tcp_ctx_t *sctx, int op, int event, void *ptr)
{
    struct mtcp_epoll_event mev;
    mev.events = event | MTCP_EPOLLET;
    mev.data.ptr = ptr;

    if (op == MTCP_EPOLL_CTL_ADD && sctx->active) {
        op  = MTCP_EPOLL_CTL_MOD;
    }

    mtcp_lua_thread_ctx_t *ctx = sctx->vm_ctx;
    if (mtcp_epoll_ctl(ctx->main->mctx, ctx->main->ep, op, sctx->fd, &mev) != 0) {
        fprintf(stderr, "mtcp_epoll_ctl failed,\n");
        return -1;
    }

    if (op == MTCP_EPOLL_CTL_DEL) {
        sctx->active = 0;
    } else {
        sctx->active = 1;
    }

    return 0;
}


static int mtcp_lua_socket_tcp(lua_State *L)
{
    int n = lua_gettop(L);
    if (n != 0) {
        return luaL_error(L, "expect 0 arguments, but got %d", n);
    }

    mtcp_lua_socket_tcp_ctx_t *sctx;
    sctx = (mtcp_lua_socket_tcp_ctx_t *)lua_newuserdata(L, sizeof(*sctx));

    /*
    sctx->busy = 0;
    sctx->connected = 0;
    sctx->vm = 0;
    sctx->vm_ctx = 0;
    */
    memset(sctx, 0, sizeof(*sctx));
    sctx->fd = -1;
    sctx->connect_timeout = 6000;
    sctx->read_timeout = 6000;
    sctx->write_timeout = 6000;

    luaL_getmetatable(L, libname);
    lua_setmetatable(L, -2);

    return 1;
}


void mtcp_lua_socket_tcp_connect_handler(event_t *ev)
{
    mtcp_lua_socket_tcp_ctx_t *sctx = ev->data;
    mtcp_lua_thread_ctx_t *ctx = sctx->vm_ctx;
    lua_State *L = sctx->vm;


    if (ev->timedout) {
        mtcp_lua_epoll_ctl(sctx, EPOLL_CTL_DEL, 0, 0);
        lua_pushnil(L);
        lua_pushliteral(L, "connect timeout");
        return mtcp_lua_run_thread(ctx, 2);
    } else {
        event_del_timer(ctx->main, ev);
    }

    int err = 0;
    socklen_t len = sizeof(int);
    if (mtcp_getsockopt(ctx->main->mctx, sctx->fd, SOL_SOCKET, SO_ERROR, (void*)&err, &len) == -1) {
        err = errno;
    }
    //fprintf(stderr, "fd:%d.\n", sctx->fd);
    if (err) {
        printf("connect failed, errno:%d. err:%s.\n", err, strerror(err));
        return;
    }

    sctx->connected = 1;

    lua_pushboolean(L, 1);
    lua_pushliteral(L, "connect successful");
    return mtcp_lua_run_thread(ctx, 2);
}


static int
mtcp_lua_socket_tcp_connect(lua_State *L)
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

    mtcp_lua_thread_ctx_t *ctx = mtcp_lua_thread_get_ctx(L);

    struct sockaddr_in  *addr = (struct sockaddr_in *)&sctx->sockaddr;
    sctx->socklen = sizeof(struct sockaddr);
    memset(addr, 0, sizeof(*addr));
    addr->sin_family = AF_INET;

    const char *host = luaL_checkstring(L, 2);
    int ret = inet_pton(AF_INET, host, &addr->sin_addr.s_addr);
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
    addr->sin_port = htons(port);

    int fd;
    fd = mtcp_socket(ctx->main->mctx, AF_INET, SOCK_STREAM, 0);
    if (fd < 0) {
        lua_pushnil(L);
        lua_pushliteral(L, "create socket failed");
        return 2;
    }

    //set socket unblocking
    ret = mtcp_setsock_nonblock(ctx->main->mctx, fd);
    if (ret < 0) {
        lua_pushnil(L);
        lua_pushliteral(L, "set noblock failed");
        return 2;
    }

    ret = mtcp_connect(ctx->main->mctx, fd, &sctx->sockaddr, sctx->socklen);
    if (ret == 0) {
        sctx->fd = fd;
        lua_pushboolean(L, 1);
        return 1;
    }

    int err;
    err = errno;
    //fprintf(stderr, "connect ret:%d, err:%d\n", ret, err);
    if (err != EINPROGRESS && err != EAGAIN) {
        printf("ret:%d, errno:%d, err:%s.\n", ret, errno, strerror(errno));
        mtcp_lua_socket_tcp_close_helper(ctx, sctx);

        lua_pushnil(L);
        lua_pushliteral(L, "connct failed.");
        return 2;
    }

    sctx->fd = fd;
    sctx->vm = L;
    sctx->vm_ctx = ctx;

    event_t *ev = &ctx->ev;
    ev->data = sctx;
    ev->timedout = 0;
    ev->handler = mtcp_lua_socket_tcp_connect_handler;

    ret = mtcp_lua_epoll_ctl(sctx, MTCP_EPOLL_CTL_ADD, MTCP_EPOLLOUT, ev);
    if (ret != 0) {
        mtcp_lua_socket_tcp_close_helper(ctx, sctx);
        sctx->fd = -1;
        sctx->connected = 0;
        lua_pushnil(L);
        lua_pushliteral(L, "mtcp epoll ctl failed.");
        return 2;
    }

    event_add_timer(ctx->main, ev, sctx->connect_timeout);
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

    mtcp_lua_thread_ctx_t *ctx;
    ctx = mtcp_lua_thread_get_ctx(L);

    size_t len;
    const char *data = luaL_checklstring(L, 2, &len);
    if (mtcp_write(ctx->main->mctx, sctx->fd, data, len) != len) {
        lua_pushnil(L);
        lua_pushliteral(L, "write socket failed");
        return 2;
    }

    lua_pushboolean(L, 1);
    return 1;
}


void mtcp_lua_socket_tcp_recv_handler(event_t *ev)
{
    //printf("Come here.\n");
    mtcp_lua_socket_tcp_ctx_t *sctx = ev->data;
    lua_State *L = sctx->vm;
    mtcp_lua_thread_ctx_t *ctx = mtcp_lua_thread_get_ctx(L);
    if (ev->timedout) {
        mtcp_lua_epoll_ctl(sctx, EPOLL_CTL_DEL, 0, 0);
        lua_pushnil(L);
        lua_pushliteral(L, "recv timedout");
        return mtcp_lua_run_thread(ctx, 2);
    } else {
        event_del_timer(ctx->main, ev);
    }

    int n;
    char buffer[1024];
    size_t sum = 0;
    
    luaL_Buffer b;
    luaL_buffinit(L, &b);
    while (1) {
        n = mtcp_read(ctx->main->mctx, sctx->fd, buffer, 1024);
        //fprintf(stderr, "recv read: %d.\n", sctx->fd);
        if (n > 0) {
            luaL_addlstring(&b, buffer, n);
            sum += n;

        } else {
            break;
        }
    }

    if (n == -1 && errno != EAGAIN) {
        lua_pushnil(L);
        lua_pushliteral(L, "recv failed");
        return mtcp_lua_run_thread(ctx, 2);
    }

    if (sum == 0) {
        lua_pushnil(L);
        lua_pushliteral(L, "closed");
        return mtcp_lua_run_thread(ctx, 2);
    }
    if (sum > 0)
        luaL_pushresult(&b);
    else
        lua_pushnil(L);
    
    if (n == 0) {
        lua_pushliteral(L, "closed");
        return mtcp_lua_run_thread(ctx, 2);
    }

    return mtcp_lua_run_thread(ctx, 1);
}


static int
mtcp_lua_socket_tcp_settimeout(lua_State *L)
{
    int n = lua_gettop(L);
    if (n != 2) {
        return luaL_error(L, "expect 2 arguments, but got %d", n);
    }

    mtcp_lua_socket_tcp_ctx_t *sctx;
    sctx = (mtcp_lua_socket_tcp_ctx_t *)lua_touserdata(L, 1);

    int timeout = luaL_checkint(L, 2);
    if (timeout <= 0 || timeout > 600000) {
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

    char buffer[8192];
    mtcp_lua_thread_ctx_t *ctx = mtcp_lua_thread_get_ctx(L);
    n = mtcp_read(ctx->main->mctx, sctx->fd, buffer, 8192);
    if (n > 0) {
        //fprintf(stderr, "fd: %d, ret: %d, %s.\n", sctx->fd, n, buffer);
        lua_pushlstring(L, buffer, n);
        return 1;
    }

    if (errno != EAGAIN) {
        lua_pushnil(L);
        lua_pushliteral(L, "read socket failed");
        return 2;
    }

    sctx->vm = L;
    sctx->vm_ctx = ctx;
    //sctx->busy = 1;
    
    event_t *ev = &ctx->ev;
    ev->data = sctx;
    ev->timedout = 0;
    ev->handler = mtcp_lua_socket_tcp_recv_handler;

    event_add_timer(ctx->main, ev, sctx->read_timeout);
    //mtcp_lua_epoll_ctl(sctx, MTCP_EPOLL_CTL_DEL, 0, 0);
    //fprintf(stderr, "fd:%d.\n", sctx->fd);
    mtcp_lua_epoll_ctl(sctx, MTCP_EPOLL_CTL_ADD, MTCP_EPOLLIN, ev);

   // printf("recv yield.\n");
    return lua_yield(L, 0);
}


static int
mtcp_lua_socket_tcp_close_helper(mtcp_lua_thread_ctx_t *ctx,
    mtcp_lua_socket_tcp_ctx_t *sctx)
{
    if (sctx->fd != -1) {
        if (sctx->active) {
            mtcp_lua_epoll_ctl(sctx, MTCP_EPOLL_CTL_DEL, 0, 0);
        }
        mtcp_close(ctx->main->mctx, sctx->fd);
    }

    return 1;
}


static int mtcp_lua_socket_tcp_close(lua_State *L)
{
    mtcp_lua_socket_tcp_ctx_t *sctx;
    sctx = (mtcp_lua_socket_tcp_ctx_t *)lua_touserdata(L, 1);
    mtcp_lua_thread_ctx_t *ctx = sctx->vm_ctx;

    if (mtcp_lua_socket_tcp_close_helper(ctx, sctx) != 1) {
        lua_pushboolean(L, 0);
        return 1;
    }

    lua_pushboolean(L, 1);
    return 1;
}


static int mtcp_lua_socket_tcp_gc(lua_State *L) {
    mtcp_lua_socket_tcp_ctx_t *sctx;
    sctx = (mtcp_lua_socket_tcp_ctx_t *)lua_touserdata(L, 1);
    mtcp_lua_thread_ctx_t *ctx = sctx->vm_ctx;

    mtcp_lua_socket_tcp_close_helper(ctx, sctx);

    //printf("gcc called.\n");
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

/*
 * lmbuf.c
 *
 *  Created on: 2016年8月20日
 *      Author: arch
 */


#include <lua.h>
#include <lualib.h>
#include <lauxlib.h>
#include "lmbuf.h"
#include "lconf.h"

#include <rte_ethdev.h>
#include <rte_mbuf.h>



typedef struct {
	struct rte_mbuf *mbuf;
} lmbuf;

static const char *libname = "ldpdk-mbuf";

static int ldpdk_mbuf_gc(lua_State *L) {
	if (luaL_checkudata(L, 1, libname) == NULL) {
		return luaL_error(L, "Not userdata");
	}

	lmbuf *mf = (lmbuf *)lua_touserdata(L, 1);
	struct rte_mbuf *mbuf = mf->mbuf;
	while (mbuf) {
		struct rte_mbuf *next;
		next = mbuf->next;
		rte_pktmbuf_free(mbuf);
		mbuf = next;
	}

	lua_pushboolean(L, 1);
	return 1;
}


static int ldpdk_mbuf_get(lua_State *L) {
	if (lua_gettop(L) != 0) {
		luaL_error(L, "Need not arguents");
	}

    uint8_t core_id = rte_lcore_id();
    struct rte_mempool *pool = ldpdk_core_conf[core_id].mempool;

	struct rte_mbuf *pkt;
	pkt = rte_pktmbuf_alloc(pool);
	if (pkt == NULL) {
		luaL_error(L, "No memory avaliable");
	}

	//printf("pkt->next:%p,data_off:%u, data_len:%u,buf_len:%u\n", pkt->next, pkt->data_off, pkt->data_len, pkt->buf_len);
	lmbuf *lm = (lmbuf *)lua_newuserdata(L, sizeof(lmbuf));
	lm->mbuf = pkt;

	luaL_getmetatable(L, libname);
	lua_setmetatable(L, -2);


	return 1;
}

static int ldpdk_mbuf_set(lua_State *L) {
	int n = lua_gettop(L);
	if (n != 2) {
		return luaL_error(L, "expect 2 arguments but got %d", n);
	}

	lmbuf *pkt = luaL_checkudata(L, -2, libname);
	if (pkt == NULL) {
		return luaL_error(L, "Not a mbuf userdata");
	}

	const char *data;
	size_t len;
	data = luaL_checklstring(L, -1, &len);

	struct rte_mbuf *mbuf= pkt->mbuf;
	memcpy(mbuf->buf_addr + mbuf->data_off, data, len);
	mbuf->data_len = len;
	mbuf->pkt_len = len;

	//printf("date len:%lu.\n", len);

	return 0;

}


static int ldpdk_mbuf_send(lua_State *L) {
	lmbuf *pkt = luaL_checkudata(L, -1, libname);
	if (pkt == NULL) {
		return luaL_error(L, "Not a mbuf userdata");
	}

    uint8_t core_id = rte_lcore_id();
    int count = rte_eth_tx_burst(ldpdk_core_conf[core_id].port_id, ldpdk_core_conf[core_id].queue_id, &pkt->mbuf, 1);
    if (unlikely(count != 1)) {
    	printf("Here.\n");
        rte_pktmbuf_free(pkt->mbuf);
        pkt->mbuf = NULL;
    }

    lua_pushboolean(L, 1);
    return 1;

}

int ldpdk_inject_mbuf_api(lua_State *L) {

	lua_pushcfunction(L, ldpdk_mbuf_get);
	lua_setfield(L, -2, "mbuf");


	luaL_newmetatable(L, libname);

	lua_pushcfunction(L, ldpdk_mbuf_set);
	lua_setfield(L, -2, "set");

	lua_pushcfunction(L, ldpdk_mbuf_send);
	lua_setfield(L, -2, "send");

	lua_pushcfunction(L, ldpdk_mbuf_gc);
	lua_setfield(L, -2, "__gc");

	lua_pushvalue(L, -1);
	lua_setfield(L, -2, "__index");
	lua_pop(L, 1);

	return 1;
}

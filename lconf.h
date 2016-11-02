//
// Created by arch on 16-9-3.
//

#ifndef LUA_DPDK_LCONF_H
#define LUA_DPDK_LCONF_H

#include <lua.h>
#include <stdint.h>

#include <rte_keepalive.h>

struct core_conf {
    uint8_t                 core_id;
    uint8_t                 port_id;
    uint8_t                 queue_id;
    uint8_t                 socket;
    struct rte_mempool      *mempool;

};

extern struct core_conf ldpdk_core_conf[RTE_MAX_LCORE];

int ldpdk_inject_conf_api(lua_State *L);

#endif //LUA_DPDK_LCONF_H

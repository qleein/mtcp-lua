//
// Created by arch on 16-9-3.
//

#include <lua.h>
#include <lauxlib.h>

#include "lconf.h"

#include <string.h>
#include <unistd.h>

#include <rte_ethdev.h>


const struct rte_eth_conf default_rte_eth_conf = {
        .rxmode = {
                .split_hdr_size = 0,
                .header_split   = 0,	/**< Header Split disabled. */
                .hw_ip_checksum = 0,	/**< IP checksum offload disabled. */
                .hw_vlan_filter = 0,	/**< VLAN filtering enabled. */
                .hw_vlan_strip  = 0,	/**< VLAN strip enabled. */
                .hw_vlan_extend = 0,	/**< Extended VLAN disabled. */
                .jumbo_frame    = 0,	/**< Jumbo Frame Support disabled. */
                .hw_strip_crc   = 0,	/**< CRC stripping by hardware disabled. */
        },
        .rx_adv_conf = {
                .rss_conf = {
                        .rss_key = NULL,
                        .rss_hf = ETH_RSS_IP,
                },
        },
        .txmode = {
                .mq_mode = ETH_MQ_TX_NONE,
        },
};

const struct rte_eth_rxconf default_rx_conf = {
        .rx_thresh = {
                .pthresh = 16,
                .hthresh = 16,
                .wthresh = 16,
        },
        .rx_free_thresh = 64,
        .rx_drop_en = 0,
        .rx_deferred_start = 0,
};


static uint16_t rx_rings = 32;
static uint16_t tx_rings = 32;
static uint32_t nb_rxd = 256;
static uint32_t nb_txd = 128;


#define PKT_RX_NB		8192

struct core_conf ldpdk_core_conf[RTE_MAX_LCORE];
struct rte_mempool *socket_mempool[RTE_MAX_NUMA_NODES];
static char saved_core_mask[64];

static int core_mask_conv(int ncore) {
    if (ncore < 2 || ncore > 128)
        return -1;

    int n = ncore / 4;
    int mod = ncore % 4;
    int pos = 0;

    char *p = saved_core_mask;
    p[pos++] = '0';
    p[pos++] = 'x';
    switch (mod) {
        case 1:
            p[pos++] = '1';
            break;
        case 2:
            p[pos++] = '3';
            break;
        case 3:
            p[pos++] = '7';
            break;
        default:
            break;
    }

    while (n--) {
        p[pos++] = 'f';
    }
    p[pos] = '\0';
    return 0;
}


static int ldpdk_init(lua_State *L) {
    int n = lua_gettop(L);
    if (n != 1) {
        return luaL_error(L, "expect 1 arguments but got %d", n);
    }

    int ncore = luaL_checkint(L, 1);
    if (core_mask_conv(ncore) != 0) {
        return luaL_error(L, "invalid core num.");
    }

    char *argv_[] = { (char *)"lua-dpdk", (char *)"-c", (char *)saved_core_mask };

    int ret = rte_eal_init(sizeof(argv_) / sizeof(char *), argv_);
    if (ret < 0)
        return luaL_error(L, "rte_eal_init failed");

    //int rx_rings, tx_rings;
    rx_rings = ncore - 1;
    tx_rings = ncore - 1;

    uint8_t nb_ports = rte_eth_dev_count();
    socket_mempool[0] = rte_pktmbuf_pool_create("ldpdk socket mempool0",
                                             PKT_RX_NB,
                                             64,
                                             0,
                                             RTE_MBUF_DEFAULT_BUF_SIZE,
                                             0);
    if (socket_mempool[0] == NULL) {
        return luaL_error(L, "create socket mempool failed");
    }

    uint8_t port_id;
    for (port_id = 0; port_id < nb_ports; port_id++) {
        ret = rte_eth_dev_configure(port_id, rx_rings, tx_rings, &default_rte_eth_conf);
        if (ret != 0) {
            return luaL_error(L, "port %u configure failed", port_id);
        }

        int queue_id;
        for (queue_id = 0; queue_id < rx_rings; queue_id++) {
            ret = rte_eth_rx_queue_setup(port_id, queue_id, nb_rxd, 0, &default_rx_conf, socket_mempool[0]);
            if (ret < 0) {
                return luaL_error(L, "rx_queue_setup failed on port %u, queue:%d, ret:%d", port_id, queue_id, ret);
            }
        }

        for (queue_id = 0; queue_id < tx_rings; queue_id++) {
            ret = rte_eth_tx_queue_setup(port_id, queue_id, nb_txd, 0, NULL);
            if (ret < 0) {
                return luaL_error(L, "tx_queue_setup failed on port %u, queue:%d, ret:%d", port_id, queue_id, ret);
            }
        }

        ret = rte_eth_dev_start(port_id);
        if (ret < 0)
            return luaL_error(L, "start port %u failed", port_id);

    }

    memset(ldpdk_core_conf, 0, sizeof(ldpdk_core_conf));
    int i = 0;
    for (i = 0; i <= ncore; i++) {
        struct core_conf *cf = &ldpdk_core_conf[i];
        cf->core_id = i;
        cf->queue_id = i - 1;
        cf->port_id = 0;
        cf->mempool = socket_mempool[0];
    }

    return 0;
}


int ldpdk_inject_conf_api(lua_State *L) {

    lua_pushcfunction(L, ldpdk_init);
    lua_setfield(L, -2, "init");

    return 1;
}

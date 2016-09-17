/*
 * ldpdk.c
 *
 *  Created on: 2016年8月20日
 *      Author: arch
 */


#include <lua.h>
#include "ldpdk.h"
#include "lmbuf.h"
#include "lconf.h"


int ldpdk_inject_dpdk_api(lua_State *L, int flags) {

	lua_createtable(L, 8, 8);

	lua_pushstring(L, "Hello World!");
	lua_setfield(L, -2, "str");

	if (flags)
		ldpdk_inject_mbuf_api(L);
    else
		ldpdk_inject_conf_api(L);

	lua_setglobal(L, "dpdk");

	return 1;
}

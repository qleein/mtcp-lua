/*
 * ldpdk.h
 *
 *  Created on: 2016年8月20日
 *      Author: arch
 */

#ifndef _MTCP_LUA_SOCKET_TCP_H_
#define _MTCP_LUA_SOCKET_TCP_H_


#include <lua.h>

#define LUA_API_MASTER 0
#define LUA_API_SLAVE  1



int mtcp_lua_inject_socket_tcp_api(lua_State *);

#endif /* LDPDK_H_ */

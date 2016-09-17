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


struct mtcp_lua_ctx_s {
    lua_State           *L;

}



int mtcp_lua_inject_api(lua_State *L, int flags) {
    return 1;
}


static lua_State *lua_init(int);

static lua_State *
lua_init(int flags) {
	lua_State *L = lua_open();
	if (!L) return NULL;

	luaL_openlibs(L);

	if (mtcp_lua_inject_api(L, flags) != 1) {
		printf("inject api failed.\n");
		return NULL;
	}

	return L;
}

int
main(int argc, char *argv[])
{
	lua_State *L = lua_init(0);
	if (!L) return 1;

	int ret = luaL_loadfile(L, "test.lua");
	if (ret != 0) {
		printf("execute lua file failed:%d.\n",ret);
		printf("res:%s.\n", luaL_checkstring(L, 1));
        exit(1);
	}

	//ret = lua_pcall(L, 0, LUA_MULTRET, 0);
	ret = lua_resume(L, 0);
    switch (ret) {
    case LUA_YIELD:
        printf("yield.\n");
        break;
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
        exit(1);
	}

	lua_close(L);

	printf("Come here.\n");

	return 0;
}

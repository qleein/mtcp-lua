#ifndef __MTCP_LUA_LOG_H__
#define __MTCP_LUA_LOG_H__


#define LOG_LEVEL_DEBUG     1
#define LOG_LEVEL_INFO      2
#define LOG_LEVEL_WARN      3
#define LOG_LEVEL_ERR       4
#define LOG_LEVEL_FATAL     5

#define MAX_LOG_LENGTH      1024


extern void mtcp_lua_log_init(int, const char *);
extern void mtcp_lua_log(int level, const char *fmt, ...);

#define _LOG_F_F_L_CS   __FILE__, __FUNCTION__, __LINE__

#define DDEBUG

#ifdef DDEBUG
    #define DEBUG(...)  mtcp_lua_log(LOG_LEVEL_DEBUG, __VA_ARGS__);
#else
    #define DEBUG(...)
#endif

#define INFO(...)   mtcp_lua_log(LOG_LEVEL_INFO, __VA_ARGS__)
#define WARN(...)   mtcp_lua_log(LOG_LEVEL_WARN, __VA_ARGS__)
#define ERROR(...)   mtcp_lua_log(LOG_LEVEL_ERR, __VA_ARGS__)
#define FATAL(...)   mtcp_lua_log(LOG_LEVEL_FATAL, __VA_ARGS__)

 
#endif

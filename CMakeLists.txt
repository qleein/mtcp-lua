cmake_minimum_required(VERSION 2.8)
project(mtcp-lua)

set(CMAKE_VERBOSE_MAKEFILE true)
set(CMAKE_C_COMPILER gcc)

# mtcp directory
set(MTCP_DIR "/root/mtcp")

file(STRINGS ${MTCP_DIR}/dpdk/lib/ldflags.txt DPDK_LD_FLAGS)
set(CMAKE_C_FLAGS " ${DPDK_LD_FLAGS} ")

set(MTCP_FLD ${MTCP_DIR}/mtcp)
set(MTCP_INC ${MTCP_FLD}/include)
set(MTCP_LIB ${MTCP_FLD}/lib)
set(UTIL_FLD ${MTCP_DIR}/util)
set(UTIL_INC ${UTIL_FLD}/include)

include_directories(${UTIL_INC} ${MTCP_INC} ${UTIL_FLD}/include)
link_directories(${MTCP_LIB})

include_directories(${CMAKE_SOURCE_DIR}/inc)
link_directories(${CMAKE_SOURCE_DIR}/lib)
link_libraries(dl m luajit mtcp numa pthread)

set(SOURCE_FILES
    main.c
    mtcp_lua_socket_tcp.h
    mtcp_lua_socket_tcp.c
    mtcp_lua_log.h
    mtcp_lua_log.c
    event_timer.h
    event_timer.c
    rbtree.h
    rbtree.c
    )

add_executable(mtcp_lua ${SOURCE_FILES})

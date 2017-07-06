cmake_minimum_required(VERSION 2.8)
project(mtcp-lua)

set(CMAKE_C_COMPILER gcc)
set(CMAKE_C_FLAGS "-g -O0")
set(CMAKE_LD_FLAGS "-lluajit-5.1")

set(DPDK_INC "/root/mtcp/dpdk/include")
set(DPDK_LIB "/root/mtcp/dpdk/lib")
set(MTCP_FLD "/root/mtcp/mtcp")
set(MTCP_INC ${MTCP_FLD}/include)
set(MTCP_LIB ${MTCP_FLD}/lib)
set(MTCP_TARGET "/libmtcp.a")
set(UTIL_FLD "/root/mtcp/util")
set(UTIL_INC ${UTIL_FLD}/include)
set(UTIL_OBJ ${UTIL_FLD}/http_parsing.o ${UTIL_FLD}/tdate_parse.o)

include_directories(${UTIL_INC} ${MTCP_INC} ${UTIL_FLD}/include)
link_directories(${MTCP_LIB})

include_directories(${MTCP_INC})

#include_directories(-march=native -DRTE_MACHINE_CPUFLAG_SSE -DRTE_MACHINE_CPUFLAG_SSE2 -DRTE_MACHINE_CPUFLAG_SSE3 -DRTE_MACHINE_CPUFLAG_SSSE3 -DRTE_MACHINE_CPUFLAG_SSE4_1 -DRTE_MACHINE_CPUFLAG_SSE4_2 -DRTE_MACHINE_CPUFLAG_AES -DRTE_MACHINE_CPUFLAG_PCLMULQDQ -DRTE_MACHINE_CPUFLAG_AVX -DRTE_MACHINE_CPUFLAG_RDRAND -DRTE_COMPILE_TIME_CPUFLAGS=RTE_CPUFLAG_SSE,RTE_CPUFLAG_SSE2,RTE_CPUFLAG_SSE3,RTE_CPUFLAG_SSSE3,RTE_CPUFLAG_SSE4_1,RTE_CPUFLAG_SSE4_2,RTE_CPUFLAG_AES,RTE_CPUFLAG_PCLMULQDQ,RTE_CPUFLAG_AVX,RTE_CPUFLAG_RDRAND)

link_directories(${CMAKE_SOURCE_DIR}/lib)
link_libraries(dl m luajit)

set(SOURCE_FILES
    main.c
    mtcp_lua_socket_tcp.h
    mtcp_lua_socket_tcp.c
    event_timer.h
    event_timer.c
    rbtree.h
    rbtree.c
    )

add_executable(mtcp_lua ${SOURCE_FILES})

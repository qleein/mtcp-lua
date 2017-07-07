cmake_minimum_required(VERSION 2.8)
project(mtcp-lua)

set(CMAKE_VERBOSE_MAKEFILE true)
set(CMAKE_C_COMPILER gcc)
set(CMAKE_C_FLAGS "-g -O0")
#set(CMAKE_LD_FLAGS "-lluajit-5.1")
#set(CMAKE_C_FLAGS " -g -O0 -L/root/mtcp/dpdk-16.11/x86_64-native-linuxapp-gcc/lib -Wl,-lrte_kni -Wl,-lrte_pipeline -Wl,-lrte_table -Wl,-lrte_port -Wl,-lrte_pdump -Wl,-lrte_distributor -Wl,-lrte_reorder -Wl,-lrte_ip_frag -Wl,-lrte_meter -Wl,-lrte_sched -Wl,-lrte_lpm -Wl,--whole-archive -Wl,-lrte_acl -Wl,--no-whole-archive -Wl,-lrte_jobstats -Wl,-lrte_power -Wl,--whole-archive -Wl,-lrte_timer -Wl,-lrte_hash -Wl,-lrte_vhost -Wl,-lrte_kvargs -Wl,-lrte_mbuf -Wl,-lrte_net -Wl,-lrte_ethdev -Wl,-lrte_cryptodev -Wl,-lrte_mempool -Wl,-lrte_ring -Wl,-lrte_eal -Wl,-lrte_cmdline -Wl,-lrte_cfgfile -Wl,-lrte_pmd_bond -Wl,-lrte_pmd_af_packet -Wl,-lrte_pmd_bnxt -Wl,-lrte_pmd_cxgbe -Wl,-lrte_pmd_e1000 -Wl,-lrte_pmd_ena -Wl,-lrte_pmd_enic -Wl,-lrte_pmd_fm10k -Wl,-lrte_pmd_i40e -Wl,-lrte_pmd_ixgbe -Wl,-lrte_pmd_null -Wl,-lrte_pmd_qede -Wl,-lrte_pmd_ring -Wl,-lrte_pmd_virtio -Wl,-lrte_pmd_vhost -Wl,-lrte_pmd_vmxnet3_uio -Wl,-lrte_pmd_null_crypto -Wl,--no-whole-archive -Wl,-lrt -Wl,-lm -Wl,-ldl ")

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
set(DPDK_LIB_FLAGS `sh cat /root/mtcp/dpdk/lib/ldflags.txt`))
message(STATUS ${DPDK_LIB_FLAGS})
#add_custom_command(OUTPUT ${DPDK_LIB_FLAGS}
#    COMMAND cat /root/mtcp/dpdk/lib/ldflags.txt
#    )

#include_directories(-march=native -DRTE_MACHINE_CPUFLAG_SSE -DRTE_MACHINE_CPUFLAG_SSE2 -DRTE_MACHINE_CPUFLAG_SSE3 -DRTE_MACHINE_CPUFLAG_SSSE3 -DRTE_MACHINE_CPUFLAG_SSE4_1 -DRTE_MACHINE_CPUFLAG_SSE4_2 -DRTE_MACHINE_CPUFLAG_AES -DRTE_MACHINE_CPUFLAG_PCLMULQDQ -DRTE_MACHINE_CPUFLAG_AVX -DRTE_MACHINE_CPUFLAG_RDRAND -DRTE_COMPILE_TIME_CPUFLAGS=RTE_CPUFLAG_SSE,RTE_CPUFLAG_SSE2,RTE_CPUFLAG_SSE3,RTE_CPUFLAG_SSSE3,RTE_CPUFLAG_SSE4_1,RTE_CPUFLAG_SSE4_2,RTE_CPUFLAG_AES,RTE_CPUFLAG_PCLMULQDQ,RTE_CPUFLAG_AVX,RTE_CPUFLAG_RDRAND)

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

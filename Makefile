TARGETS = mtcp-lua
CC=gcc -g -O2
DPDK=1
PS=0
NETMAP=0

#mtcp directory
MTCP_DIR=/root/mtcp

# DPDK LIBRARY and HEADER
DPDK_INC=${MTCP_DIR}/dpdk/include
DPDK_LIB=${MTCP_DIR}/dpdk/lib/

# mtcp library and header 
MTCP_FLD    =${MTCP_DIR}/mtcp/
MTCP_INC    =-I${MTCP_FLD}/include
MTCP_LIB    =-L${MTCP_FLD}/lib
MTCP_TARGET = ${MTCP_LIB}/libmtcp.a

UTIL_FLD = ${MTCP_DIR}/util
UTIL_INC = -I${UTIL_FLD}/include
UTIL_OBJ = ${UTIL_FLD}/http_parsing.o ${UTIL_FLD}/tdate_parse.o


INC = -I./inc/ ${UTIL_INC} ${MTCP_INC} -I${UTIL_FLD}/include
LIBS = -L./lib/ ${MTCP_LIB}

# CFLAGS for DPDK-related compilation
INC += ${MTCP_INC}
ifeq ($(DPDK),1)
DPDK_MACHINE_FLAGS = $(shell cat /root/mtcp/dpdk/include/cflags.txt)
INC += ${DPDK_MACHINE_FLAGS} -I${DPDK_INC} -include $(DPDK_INC)/rte_config.h
endif

ifeq ($(DPDK),1)
DPDK_LIB_FLAGS = $(shell cat ${MTCP_DIR}/dpdk/lib/ldflags.txt)
#LIBS += -m64 -g -O3 -pthread -lrt -march=native -Wl,-export-dynamic ${MTCP_FLD}/lib/libmtcp.a -L../../dpdk/lib -Wl,-lnuma -Wl,-lmtcp -Wl,-lpthread -Wl,-lrt -Wl,-ldl -Wl,${DPDK_LIB_FLAGS}
LIBS += -m64 -g -O3 -pthread -lrt -march=native -export-dynamic ${MTCP_FLD}/lib/libmtcp.a -L/root/mtcp/dpdk/lib -lnuma -lmtcp -lpthread -lrt -ldl ${DPDK_LIB_FLAGS} -lluajit

else
#LIBS += -m64 -g -O3 -pthread -lrt -march=native -Wl,-export-dynamic ${MTCP_FLD}/lib/libmtcp.a -L../../dpdk/lib -Wl,-lnuma -Wl,-lmtcp -Wl,-lpthread -Wl,-lrt -Wl,-ldl -Wl,${DPDK_LIB_FLAGS}
LIBS += -m64 -g -O3 -pthread -lrt -march=native -export-dynamic ${MTCP_FLD}/lib/libmtcp.a -L/root/mtcp/dpdk/lib -lnuma -lmtcp -lpthread -lrt -ldl ${DPDK_LIB_FLAGS}
endif

all: mtcp_lua

main.o: main.c main.h event_timer.h mtcp_lua_socket_tcp.h event.h mtcp_lua_log.h
	${CC} -c $< ${CFLAGS} ${INC}
event_timer.o: event_timer.c rbtree.h event_timer.h event.h
	${CC} -c $< ${CFLAGS} ${INC}
mtcp_lua_socket_tcp.o: mtcp_lua_socket_tcp.c main.h event_timer.h event.h
	${CC} -c $< ${CFLAGS} ${INC}
rbtree.o: rbtree.c rbtree.h
	${CC} -c $< ${CFLAGS} ${INC}
mtcp_lua_log.o: mtcp_lua_log.c mtcp_lua_log.h
	${CC} -c $< ${CFLAGS} ${INC}
mtcp_lua: main.o mtcp_lua_socket_tcp.o event_timer.o rbtree.o mtcp_lua_log.o
	${CC} $^ ${LIBS} ${UTIL_OBJ} -o $@

clean:
	rm -f *~ *.o ${TARGETS} log_*

distclean: clean
	rm -rf Makefile

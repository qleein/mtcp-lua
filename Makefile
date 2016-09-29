TARGETS = mtcp-lua
CC=gcc -g -O3
DPDK=1
PS=0
NETMAP=0

# DPDK LIBRARY and HEADER
DPDK_INC=/root/mtcp/dpdk/include
DPDK_LIB=/root/mtcp/dpdk/lib/

# mtcp library and header 
MTCP_FLD    =/root/mtcp/mtcp/
MTCP_INC    =-I${MTCP_FLD}/include
MTCP_LIB    =-L${MTCP_FLD}/lib
MTCP_TARGET = ${MTCP_LIB}/libmtcp.a

UTIL_FLD = /root/mtcp/util
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
DPDK_LIB_FLAGS = $(shell cat /root/mtcp/dpdk/lib/ldflags.txt)
#LIBS += -m64 -g -O3 -pthread -lrt -march=native -Wl,-export-dynamic ${MTCP_FLD}/lib/libmtcp.a -L../../dpdk/lib -Wl,-lnuma -Wl,-lmtcp -Wl,-lpthread -Wl,-lrt -Wl,-ldl -Wl,${DPDK_LIB_FLAGS}
LIBS += -m64 -g -O3 -pthread -lrt -march=native -export-dynamic ${MTCP_FLD}/lib/libmtcp.a -L/root/mtcp/dpdk/lib -lnuma -lmtcp -lpthread -lrt -ldl ${DPDK_LIB_FLAGS}

else
#LIBS += -m64 -g -O3 -pthread -lrt -march=native -Wl,-export-dynamic ${MTCP_FLD}/lib/libmtcp.a -L../../dpdk/lib -Wl,-lnuma -Wl,-lmtcp -Wl,-lpthread -Wl,-lrt -Wl,-ldl -Wl,${DPDK_LIB_FLAGS}
LIBS += -m64 -g -O3 -pthread -lrt -march=native -export-dynamic ${MTCP_FLD}/lib/libmtcp.a -L/root/mtcp/dpdk/lib -lnuma -lmtcp -lpthread -lrt -ldl ${DPDK_LIB_FLAGS}
endif


main.o: main.c
	${CC} -c $< ${CFLAGS} ${INC}
mtcp_lua_socket_tcp.o: mtcp_lua_socket_tcp.c
	${CC} -c $< ${CFLAGS} ${INC}
event_timer.o: event_timer.c
	${CC} -c $< ${CFLAGS} ${INC}
rbtree.o: rbtree.c
	${CC} -c $< ${CFLAGS} ${INC}
mtcp_lua: main.o mtcp_lua_socket.tcp.o event_timer.o rbtree.o
	${CC} $< ${LIBS} ${UTIL_OBJ} -o $@

clean:
	rm -f *~ *.o ${TARGETS} log_*

distclean: clean
	rm -rf Makefile

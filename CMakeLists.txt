cmake_minimum_required(VERSION 2.8)
project(mtcp-lua)

set(CMAKE_C_COMPILER gcc)
set(CMAKE_C_FLAGS "-g -O0")
set(CMAKE_LD_FLAGS "-lluajit-5.1")

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

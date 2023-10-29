cmake_minimum_required(VERSION 3.0.0)
project(rsshell VERSION 0.1.0 LANGUAGES C)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -g")

include(CTest)
enable_testing()

add_subdirectory(tests)
include_directories(${CMAKE_SOURCE_DIR}/lib/bdwgc/include/)

add_definitions(-DDEBUG)

add_library(rsshell
    header.h
    mem.h mem.c
    ast.h ast.c
    vtable.h vtable.c
    expr.h expr.c
    parser.h parser.c
    rstack.h rstack.c
    trie.h trie.c
    cmds.h cmds.c
    rfunction.h rfunction.c
    rsshell.h rsshell.c
    rsarray.h rsarray.c
    classes.h classes.c
    nativeroutines.h nativeroutines.c)

target_link_libraries(rsshell ${CMAKE_SOURCE_DIR}/lib/bdwgc/build/libgc.dll)

set(CPACK_PROJECT_NAME ${PROJECT_NAME})
set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})
include(CPack)

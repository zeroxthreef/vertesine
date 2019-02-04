cmake_minimum_required(VERSION 2.8)


include(ExternalProject)

project(vertesine)

add_definitions(-DLOG_USE_COLOR)


FILE(GLOB SRCS "${PROJECT_SOURCE_DIR}/src/*.c" "${PROJECT_SOURCE_DIR}/src/pages/*.c" "${PROJECT_SOURCE_DIR}/lib/vec/src/*.c" "${PROJECT_SOURCE_DIR}/lib/ini/src/*.c" "${PROJECT_SOURCE_DIR}/lib/map/src/*.c" "${PROJECT_SOURCE_DIR}/lib/log.c/src/*.c" "${PROJECT_SOURCE_DIR}/lib/sandbird/src/*.c" "${PROJECT_SOURCE_DIR}/lib/snowflaked/src/snowflake.c")

# TODO make an option to build without debug symbols without editing the cmakelists.txt
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -O0 -ggdb -fno-omit-frame-pointer")

add_executable(vertesine ${SRCS})

#ExternalProject_Add(MD4C SOURCE_DIR "${PROJECT_SOURCE_DIR}/lib/md4c" INSTALL_COMMAND true)
#add_dependencies(vertesine MD4C)

#target_link_libraries(vertesine hiredis jansson ${MD4C})
target_link_libraries(vertesine hiredis jansson markdown b64 gd libsodium.a)

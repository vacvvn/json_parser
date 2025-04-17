cmake_minimum_required(VERSION 2.6)
project(json_parser)

set(SOURCES JSON_parser.c)
set(CMAKE_C_FLAGS ${CMAKE_C_FLAGS} -O0 -Og)
add_executable(${PROJECT_NAME} ${SOURCES})
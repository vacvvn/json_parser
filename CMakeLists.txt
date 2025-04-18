cmake_minimum_required(VERSION 2.6)
project(json_parser)

set(SOURCES json_parser.c)
set(CMAKE_C_FLAGS ${CMAKE_C_FLAGS} -g)
# set(CMAKE_C_FLAGS -g)
add_executable(${PROJECT_NAME} ${SOURCES})
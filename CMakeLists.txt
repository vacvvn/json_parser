cmake_minimum_required(VERSION 2.6)
project(json_parser)

set(SOURCES JSON_parser.c)
add_executable(${PROJECT_NAME} ${SOURCES})
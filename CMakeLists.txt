cmake_minimum_required(VERSION 3.8)
project(untitled12)

set(CMAKE_C_STANDARD 90)

set(SOURCE_FILES main.c threadPool.h osqueue.h osqueue.c threadPool.c)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -pthread")

add_executable(untitled12 ${SOURCE_FILES})
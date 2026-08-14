# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
find_package(Python REQUIRED COMPONENTS Interpreter)

project(flowink)

FILE(GLOB app_sources 
    app/*.c
    driver/*.c
    server/*.c
    test/*.c)
target_sources(app PRIVATE ${app_sources})

target_include_directories(app PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/app
    ${CMAKE_CURRENT_SOURCE_DIR}/driver
    ${CMAKE_CURRENT_SOURCE_DIR}/server
    ${CMAKE_CURRENT_SOURCE_DIR}/test
)

set(gen_dir ${ZEPHYR_BINARY_DIR}/include/generated/)
set(read_dir ${CMAKE_CURRENT_SOURCE_DIR}/test/bmp_source/)

generate_inc_file_for_target(app
    ${read_dir}/test.bmp
    ${gen_dir}/test.bmp.inc
)

# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
find_package(Python REQUIRED COMPONENTS Interpreter)

project(flowink)

FILE(GLOB app_sources 
    app/*.c
    app/net/*.c
    driver/*.c
    server/*.c    
    test/*.c)
target_sources(app PRIVATE ${app_sources})

target_include_directories(app PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/app
    ${CMAKE_CURRENT_SOURCE_DIR}/app/net
    ${CMAKE_CURRENT_SOURCE_DIR}/driver
    ${CMAKE_CURRENT_SOURCE_DIR}/server
)

target_link_libraries(app PRIVATE zephyr_interface zephyr)

zephyr_linker_sources(SECTIONS ${CMAKE_CURRENT_SOURCE_DIR}/app/net/sections-rom.ld)
zephyr_linker_section(
  NAME http_resource_desc_http_service
  KVMA RAM_REGION GROUP RODATA_REGION
)

# 固定生成目录和读取目录
set(gen_dir ${ZEPHYR_BINARY_DIR}/include/generated/)
set(read_dir ${CMAKE_CURRENT_SOURCE_DIR}/app/net/http_source/)

# 把需要嵌入固件的网页文件名写在这里
set(WEB_FILES
    index.html
    script.js
)

# 遍历变量内所有内容，‘&’取出变量值——变量file=变量值
foreach(file ${WEB_FILES})

# 读取一个二进制文件，压缩后转为 C 静态数组，生成 .inc 文件：固件；源文件路径；输出路径；转换前先压缩为gzip
generate_inc_file_for_target(app
    ${read_dir}/${file}
    ${gen_dir}/${file}.gz.inc
    --gzip                
)
endforeach()

generate_inc_file_for_target(app
    ${read_dir}/zkk.png
    ${gen_dir}/zkk.png.inc
)

set(read_dir_test ${CMAKE_CURRENT_SOURCE_DIR}/test/bmp_source/)
generate_inc_file_for_target(app
    ${read_dir_test}/test.bmp
    ${gen_dir}/test.bmp.inc
)
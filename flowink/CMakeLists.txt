# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(flowink)

FILE(GLOB app_sources 
    main.c
    driver/src/*.c)
target_sources(app PRIVATE ${app_sources})

# 添加workq_common.h所在头文件目录
target_include_directories(app PRIVATE
    ${CMAKE_SOURCE_DIR}/../flowink/driver/inc
)




# 固定生成目录，Zephyr 内置头文件路径
set(gen_dir ${ZEPHYR_BINARY_DIR}/include/generated/)

# 把需要嵌入固件的网页文件名写在这里
set(WEB_FILES
    index.html
#     index.js
#     style.css
)

# 遍历变量内所有内容，‘&’取出变量值——变量file=变量值
foreach(file ${WEB_FILES})

# 读取一个二进制文件，压缩后转为 C 静态数组，生成 .inc 文件：固件；源文件路径；输出路径；转换前先压缩为gzip
generate_inc_file_for_target(app
    html/${file}
    ${gen_dir}/${file}.gz.inc
    --gzip                
)
endforeach()


# zephyr_linker_sources(SECTIONS sections-rom.ld)
# zephyr_linker_section_ifdef(CONFIG_NET_SAMPLE_HTTP_SERVICE
#   NAME http_resource_desc_test_http_service
#   KVMA RAM_REGION GROUP RODATA_REGION
# )

# target_link_libraries(app PRIVATE zephyr_interface zephyr)
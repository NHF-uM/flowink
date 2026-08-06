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




# # 2. 固定生成目录，Zephyr 内置头文件路径
# set(gen_dir ${ZEPHYR_BINARY_DIR}/include/generated/)

# # ========== 网页资源自动转.inc 核心段 ==========
# # 把需要嵌入固件的网页文件名写在这里
# set(WEB_FILES
#     index.html
#     index.js
#     style.css
# )

# foreach(file ${WEB_FILES})
# generate_inc_file_for_target(app
#     src/web/${file}        # 【重点1】你的网页存放目录：工程根目录/src/web/
#     ${gen_dir}/${file}.gz.inc
#     --gzip                 # 开启gzip压缩，浏览器自动解压
# )
# endforeach()
# # ==============================================

# zephyr_linker_sources(SECTIONS sections-rom.ld)
# zephyr_linker_section_ifdef(CONFIG_NET_SAMPLE_HTTP_SERVICE
#   NAME http_resource_desc_test_http_service
#   KVMA RAM_REGION GROUP RODATA_REGION
# )

# target_link_libraries(app PRIVATE zephyr_interface zephyr)
#ifndef _TF_H_
#define _TF_H_

#include <stdint.h>
#include <stdbool.h>

/**
 * @brief 检查 TF 卡状态、扫描并生成 dlist
 * @param disk_check_enable 是否检查磁盘状态
 * @return 0 成功
 */
int tf_init(bool disk_check_enable);

/**
 * @brief 卸载 TF 卡
 * @param  无
 */
void tf_deinit(void);

/**
 * @brief 寻找 TF 卡下第一个 BMP 文件
 * @param  无
 * @return 文件路径，返回 NULL 整个卡没有可用 BMP 文件
 */
char *tf_find_first_bmp(void);

/**
 * @brief 读取 TF 卡 BMP 文件
 * @param file_path 文件路径
 * @param bmp_buf BMP 文件缓冲区
 * @return 0 成功
 */
int tf_read_bmp(const char *file_path, uint8_t *bmp_buf);

/**
 * @brief 定位下一张图片，返回完整路径
 * 如果 loop_play=true：当前目录链表内循环播放；
 * 如果 loop_play=false：播完当前目录全部文件，切下一目录；
 * @param current_file_path 当前文件路径
 * @return 遍历完所有目录和文件 或者 tf 卡被修改（未找到当前文件）返回 NULL
 */
char *tf_find_next_bmp(const char *current_file_path);

/**
 * @brief 获取配置文件循环播放开关状态
 * @param  无
 * @return config_info->loop_play
 */
bool tf_get_loop_play(void);

/**
 * @brief 获取配置文件轮播间隔
 * @param  无
 * @return config_info->carousel_interval
 */
uint32_t tf_get_carousel_interval(void);

/**
 * @brief 获取配置文件起始路径，路径字符串为动态分配（此处无需释放！！）
 * @param  无
 * @return config_info->start_file_path
 */
char *tf_get_start_file_path(void);

/**
 * @brief 测试——打印 dlist 目录结构
 * 线程使用注意：内含 10 ms 的阻塞打印延时，避免打印过快导致串口丢失数据
 * @param 无
 */
void tf_test_ls_dlist(void);

/**
 * @brief 测试——修改循环播放开关状态
 * @param loop_play 
 */
void tf_test_change_loop_play(bool loop_play);

/**
 * @brief 测试——修改轮播间隔
 * @param interval 
 */
void tf_test_change_carousel_interval(uint32_t interval);


/* 边界测试：


// 一：根目录没有任何文件的情况，测试 find_first_bmp()

// 二：
//   /SD:
//   ├── config.txt                      # loop_subfolder=0 时用于关循环测试
//   ├── root0.bmp                       # 根目录文件（根目录链表）
//   ├── root1.bmp
//   │
//   ├── DirA/
//   │   ├── a1.bmp
//   │   ├── a2.bmp
//   │   └── a3.bmp                      # 测"链表中间/末尾"顺序
//   │
//   ├── DirB/
//   │   └── b1.bmp                      # 单文件目录：测跳目录
//   │
//   ├── DirEmpty/                       # 空目录：必须被跳过
//   │   (无任何文件)
//   │
//   └── DirIgnore/
//       └── note.txt                    # 非 .bmp 文件：必须被过滤
//       └── ignore.bmp                  # 双结尾节点


*/

#endif

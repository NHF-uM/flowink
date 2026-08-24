#ifndef _APP_TF_H_
#define _APP_TF_H_

/**
 * @brief 检查 TF 卡状态、扫描并生成 dlist
 * @param disk_check_enable 是否检查磁盘状态
 */
void tf_init(bool disk_check_enable);

/**
 * @brief 卸载 TF 卡
 * @param  无
 */
void tf_deinit(void);

char *tf_read_config_file(void);

char *tf_find_first_bmp(void);

int tf_read_bmp(const char *file_path, uint8_t **bmp_buf);

char *tf_find_next_bmp(const char *file_path);

/**
 * @brief 测试打印 dlist 目录结构
 * 线程使用注意：内含 10 ms 的阻塞打印延时，避免打印过快导致串口丢失数据
 * @param 无
 */
void tf_test_ls_dlist(void);

uint32_t tf_get_carousel_interval(void);

bool tf_get_loop_play(void);

void tf_test_change_loop_play(bool loop_play);

void tf_test_change_carousel_interval(uint32_t interval);

#endif

#ifndef _APP_TF_H_
#define _APP_TF_H_

/**
 * @brief 检查 TF 卡状态、扫描并生成 dlist
 * @param  无
 */
void tf_init(void);

/**
 * @brief 卸载 TF 卡
 * @param  无
 */
void tf_deinit(void);

char *tf_read_config_file(void);

char *tf_find_first_bmp(void);

int tf_read_bmp(const char *file_path, uint8_t **bmp_buf);

char *tf_find_next_bmp(const char *file_path);

void tf_test_ls_dlist(void);

#endif

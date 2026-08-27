#ifndef _SHOW_PICTURE_H_
#define _SHOW_PICTURE_H_ 

/**
 * @brief 在 psram 申请两个大容量 heap
 * @param ptr_bmp_buf 接收对应的 bmp_buf_heap 地址
 * @return 0 成功
 */
int show_pic_malloc(uint8_t **ptr_bmp_buf);

/**
 * @brief 释放 psram 申请的 heap
 * @param 无
 */
void show_pic_free(void);

/**
 * @brief 刷传入路径图片——>刷 start_file_path ——>刷能找到的第一张图片——>刷内置图片
 * @param path_target 传入 NULL 则直接从 start_file_path 开始查找流程
 */
void show_pic_tf_bmp(const char *path_target);

/**
 * @brief 刷服务器图片
 * @param 无
 */
void show_pic_server_bmp(void);

/**
 * @brief 刷内置图片
 * @param 无
 */
void show_pic_buildin_bmp(void);

#endif  

#ifndef _APP_NV_H_
#define _APP_NV_H_


/**
 * @brief 读取预留 RAM 区域中的文件路径
 * @param  无
 * @return 首次读取 和 读取失败 返回 NULL
 */
char *nv_read_path(void);

void nv_write_path(const char *path);

void nv_break_magic(void);

#endif 

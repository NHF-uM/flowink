#ifndef _NV_H_
#define _NV_H_


/**
 * @brief 读取预留 RAM 区域中的文件路径
 * @param  无
 * @return 首次读取 和 读取失败 返回 NULL，返回动态分配的指针，需调用 nv_free_path() 释放
 */
char *nv_read_path(void);

/**
 * @brief 释放动态分配的指针
 * @param ptr 
 */
void nv_free_path(char *ptr);

/**
 * @brief 写入文件路径
 * @param path 路径
 */
void nv_write_path(const char *path);

/**
 * @brief 破环预留 RAM 块中的魔幻校验数，表示该次未存入文件路径
 * @param  无
 */
void nv_break_magic(void);

#endif 

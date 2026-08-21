#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(main, LOG_LEVEL_DBG);

// int tf_read_config(struct carousel_info *carousel_info)
// {
//     int ret = 0;
//     char file_buf[512];

//     struct fs_file_t fd;
//     fs_file_t_init(&fd);

//     ret = fs_open(&fd, CONFIG_FILE_PATH, FS_O_READ);
//     if (ret != 0)
//     {
//         LOG_WRN("Failed to open config file, err:%d", ret);
//         goto fail;
//     }

//     // 4. 一次性读取整个配置文件到缓冲区
//     ret = fs_read(&fd, file_buf, sizeof(file_buf) - 1);
//     if (ret < 0)
//     {
//         LOG_WRN("Failed to read config file, err:%d, use default config", ret);
//         goto fail;
//     }
//     file_buf[ret] = '\0'; // 确保字符串以 '\0' 结尾

//     // 5. 逐行解析配置内容
//     char *line_ptr = file_buf;
//     char *next_line;

//     while (*line_ptr != '\0')
//     {
//         // 定位行尾换行符，分割单行
//         next_line = strchr(line_ptr, '\n');
//         if (next_line != NULL)
//         {
//             *next_line = '\0'; // 截断当前行
//             next_line++;       // 指针移动到下一行开头
//         }
//         else
//         {
//             next_line = line_ptr + strlen(line_ptr); // 处理最后一行
//         }

//         // 去除行尾回车符 \r，兼容 Windows 换行格式
//         size_t line_len = strlen(line_ptr);
//         if (line_len > 0 && line_ptr[line_len - 1] == '\r')
//         {
//             line_ptr[line_len - 1] = '\0';
//         }

//         // 跳过行首空格、制表符
//         char *p = line_ptr;
//         while (*p == ' ' || *p == '\t')
//         {
//             p++;
//         }

//         // 跳过空行和注释行（# 开头）
//         if (*p == '\0' || *p == '#')
//         {
//             line_ptr = next_line;
//             continue;
//         }

//         // 查找等号位置，分割键和值
//         char *eq_pos = strchr(p, '=');
//         if (eq_pos == NULL)
//         {
//             line_ptr = next_line; // 无等号的无效行直接跳过
//             continue;
//         }
//         *eq_pos = '\0';
//         char *key = p;
//         char *value = eq_pos + 1;

//         // 修剪 key 尾部的空格
//         char *key_end = eq_pos - 1;
//         while (key_end > key && (*key_end == ' ' || *key_end == '\t'))
//         {
//             *key_end = '\0';
//             key_end--;
//         }

//         // 修剪 value 首尾的空格
//         while (*value == ' ' || *value == '\t')
//         {
//             value++;
//         }
//         char *value_end = value + strlen(value) - 1;
//         while (value_end > value && (*value_end == ' ' || *value_end == '\t'))
//         {
//             *value_end = '\0';
//             value_end--;
//         }

//         // 提取双引号包裹的实际配置值
//         if (*value != '"' || *value_end != '"')
//         {
//             line_ptr = next_line; // 格式不符合要求，跳过该行
//             continue;
//         }
//         value++;
//         *value_end = '\0';

//         // 6. 匹配配置项并赋值，带参数合法性校验
//         if (strcmp(key, "loop_interval") == 0)
//         {
//             uint32_t interval = strtoul(value, NULL, 10);
//             if (interval >= 300 && interval <= 86400)
//             {
//                 carousel_info->carousel_interval = interval;
//             }
//             else
//             {
//                 LOG_WRN("loop_interval=%u out of range [300, 86400], keep default %u",
//                         interval, carousel_info->carousel_interval);
//                 carousel_info->carousel_interval = 0;
//             }
//         }
//         else if (strcmp(key, "loop_subfolder") == 0)
//         {
//             if (strcmp(value, "1") == 0)
//             {
//                 carousel_info->loop_play = true;
//             }
//             else if (strcmp(value, "0") == 0)
//             {
//                 carousel_info->loop_play = false;
//             }
//             else
//             {
//                 LOG_WRN("loop_subfolder invalid value '%s', keep default true", value);
//                 carousel_info->loop_play = true;
//             }
//         }
//         else if (strcmp(key, "start_file_name") == 0)
//         {
//             // 先释放之前可能分配的内存，避免泄漏
//             if (carousel_info->start_file_path != NULL)
//             {
//                 free(carousel_info->start_file_path);
//                 carousel_info->start_file_path = NULL;
//             }

//             if (strlen(value) > 0)
//             {
//                 // 拼接完整绝对路径：挂载点 + / + 配置的相对路径
//                 size_t path_size = strlen(DISK_MOUNT_PT) + 1 + strlen(value) + 1;
//                 char *full_path = malloc(path_size);
//                 if (full_path == NULL)
//                 {
//                     LOG_WRN("malloc for start_file_path failed");
//                 }
//                 else
//                 {
//                     snprintf(full_path, path_size, "%s/%s", DISK_MOUNT_PT, value);
//                     carousel_info->start_file_path = full_path;
//                 }
//             }
//         }

//         line_ptr = next_line;
//     }

//     return 0; /* 正常出口在分支前面 */

// fail:
//     carousel_info->start_file_path = NULL;
//     carousel_info->carousel_interval = 0;
//     return -1;
// }

/*
[00:00:08.314,000] <inf> sd: Maximum SD clock is under 25MHz, using clock of 24000000Hz
[00:00:08.317,000] <dbg> app_tf: tf_init: mount disk done
[00:00:08.318,000] <dbg> app_tf: scan_root_dir: [TOP DIR] System Volume Information
[00:00:08.319,000] <dbg> app_tf: scan_root_dir: [TOP DIR] bbb
[00:00:08.320,000] <dbg> app_tf: scan_sub_dir: [SUB FILE] 15c3d.bmp (size = 1152054)
[00:00:08.320,000] <dbg> app_tf: scan_sub_dir: [SUB FILE] 45ff.bmp (size = 1152054)
[00:00:08.320,000] <dbg> app_tf: scan_sub_dir: [SUB FILE] 824.bmp (size = 1152054)
[00:00:08.321,000] <dbg> app_tf: scan_root_dir: [TOP FILE] 800.bmp (size = 1152054)
[00:00:08.321,000] <dbg> app_tf: scan_root_dir: [TOP FILE] eeb.bmp (size = 1152054)
[00:00:08.321,000] <dbg> app_tf: scan_root_dir: [TOP FILE] config.txt (size = 1319)
[00:00:08.321,000] <dbg> app_tf: scan_root_dir: [TOP FILE] 1b5.bmp (size = 1152054)
[00:00:08.321,000] <dbg> app_tf: tf_deinit: umount disk done
*/

/**
 * 项目规范：
 * 1. 工具函数有返回值，顶层init函数无返回值 
 * 
 * 
*/
#include "app_tf.h"
/* tf卡的读取顺序是没有规则的，会因文件的改动而变化，所以只能用py来指定？*/
int main(void)
{
    tf_init();
    tf_deinit();
    return 0;
}

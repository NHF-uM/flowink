#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/logging/log.h>
#include <ff.h>
#include <zephyr/fs/fs.h>
#include <zephyr/storage/disk_access.h>
#include <zephyr/sys/dlist.h>
#include <string.h>
#include <stdlib.h>
LOG_MODULE_REGISTER(main, LOG_LEVEL_DBG);

#define DISK_DRIVE_NAME "SD"
#define DISK_MOUNT_PT "/" DISK_DRIVE_NAME ":"
#define CONFIG_FILE_PATH DISK_MOUNT_PT "/config.txt" // "/SD:/config.txt"

static FATFS fat_fs;

/* mounting info */
static struct fs_mount_t mp = {
    .type = FS_FATFS,
    .fs_data = &fat_fs,
};

/* 文件夹全部放到一个 list 里面，每个 node 有一个 dlist_file*/
static sys_dlist_t dlist_dir;

struct ctx_dir
{
    char dir_path[128];

    sys_dnode_t dir_node;
    sys_dlist_t dlist_file;

    uint16_t file_num;
};

struct ctx_file
{
    char file_path[128];

    sys_dnode_t file_node;
};

struct carousel_info
{
    char *start_file_path;      /* 轮播开始文件 */
    uint32_t carousel_interval; /* 轮播间隔 */
    bool loop_play;             /* 文件夹内循环播放，默认开启 */
};

/**
 * @brief 判断文件类型
 * @param str 原始字符串
 * @param suffix 后缀（例如 ".txt"）
 * @return true:匹配
 */
static bool file_endswith(const char *str, const char *suffix)
{
    if (str == NULL || suffix == NULL)
    {
        return false;
    }
    size_t str_len = strlen(str);
    size_t suf_len = strlen(suffix);

    if (str_len < suf_len)
    {
        return false;
    }
    return strcmp(str + (str_len - suf_len), suffix) == 0;
}

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
//                 size_t path_size = strlen(disk_mount_pt) + 1 + strlen(value) + 1;
//                 char *full_path = malloc(path_size);
//                 if (full_path == NULL)
//                 {
//                     LOG_WRN("malloc for start_file_path failed");
//                 }
//                 else
//                 {
//                     snprintf(full_path, path_size, "%s/%s", disk_mount_pt, value);
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

[00:00:04.123,000] <inf> sd: Maximum SD clock is under 25MHz, using clock of 24000000Hz
[00:00:04.123,000] <dbg> main: tf_check_disk: Block count 31116288
[00:00:04.123,000] <dbg> main: tf_check_disk: Sector size 512

[00:00:04.123,000] <dbg> main: tf_check_disk: Memory Size(MB) 15193

[00:00:04.123,000] <inf> main: check disk success
[00:00:08.162,000] <inf> sd: Maximum SD clock is under 25MHz, using clock of 24000000Hz
Disk mounted.
[00:00:08.165,000] <dbg> main: main: [First-level DIR ] SYSTEM~1

[00:00:08.165,000] <dbg> main: main: [First-level DIR ] BBB

[00:00:08.165,000] <dbg> main: main: [First-level FILE] 800.BMP (size = 1152054)

[00:00:08.165,000] <wrn> main: invalid file extension: 800.BMP
[00:00:08.165,000] <dbg> main: main: [First-level FILE] CONFIG.TXT (size = 1165)

[00:00:08.165,000] <wrn> main: invalid file extension: CONFIG.TXT
[00:00:08.165,000] <dbg> main: main: [First-level FILE] EEB.BMP (size = 1152054)

[00:00:08.165,000] <wrn> main: invalid file extension: EEB.BMP
[00:00:08.165,000] <dbg> main: main: Root dir scan done, total entries: 5
[00:00:08.165,000] <dbg> main: main: Root dir closed
[00:00:08.165,000] <inf> main: Disk unmounted

*/

int tf_check_disk(const char *disk_name);

void tf_init(void)
{
    int ret;

    ret = tf_check_disk(DISK_DRIVE_NAME);
    if (ret != 0)
    {
        LOG_ERR("check disk failed");
        return;
    }

    mp.mnt_point = DISK_MOUNT_PT;
    ret = fs_mount(&mp);
    if (ret != 0)
    {
        LOG_ERR("Error mounting disk, err:%d\n", ret);
        return;
    }
}

int main(void)
{
    int ret;

    tf_init();
    static const char *disk_mount_pt = DISK_MOUNT_PT;

    sys_dlist_init(&dlist_dir);

    uint16_t cnt = 0;
    struct fs_dir_t dirp;
    struct fs_dirent entry;

    fs_dir_t_init(&dirp);
    ret = fs_opendir(&dirp, disk_mount_pt);
    if (ret != 0)
    {
        LOG_ERR("Error opening root dir [%d]\n", ret);
        return ret;
    }

    struct ctx_dir *ctx_dir_root = k_malloc(sizeof(struct ctx_dir));
    if (ctx_dir_root == NULL)
    {
        LOG_ERR("malloc root dir failed");
        return -1;
    }

    strcpy(ctx_dir_root->dir_path, disk_mount_pt);

    snprintf(ctx_dir_root->dir_path, sizeof(ctx_dir_root->dir_path), "%s", disk_mount_pt);
    sys_dlist_init(&ctx_dir_root->dlist_file);
    ctx_dir_root->file_num = 0;
    sys_dlist_append(&dlist_dir, &ctx_dir_root->dir_node);

    /* 仅支持根路径下的额外一层文件夹 */
    while (1)
    {
        /* readdir 函数会自动偏移，指向 dirp 的下一个 entry */
        int ret = fs_readdir(&dirp, &entry);

        if (ret != 0 || entry.name[0] == 0)
        {
            break;
        }

        if (entry.type == FS_DIR_ENTRY_DIR)
        {
            LOG_DBG("[First-level DIR ] %s\n", entry.name);
            struct ctx_dir *ctx_dir_sub = k_malloc(sizeof(struct ctx_dir));
            if (ctx_dir_sub == NULL)
            {
                LOG_WRN("malloc sub dir failed, skip %s", entry.name);
                continue;
            }

            snprintf(ctx_dir_sub->dir_path, sizeof(ctx_dir_sub->dir_path),
                     "%s/%s", disk_mount_pt, entry.name);
            sys_dlist_init(&ctx_dir_sub->dlist_file);
            ctx_dir_sub->file_num = 0;
            sys_dlist_append(&dlist_dir, &ctx_dir_sub->dir_node);

            //         /* 解析额外一层文件夹：读取一级子文件夹内部所有文件 */
            //         struct fs_dir_t dirp_sub;
            //         struct fs_dirent entry_sub;
            //         int sub_res;

            //         fs_dir_t_init(&dirp_sub);
            //         sub_res = fs_opendir(&dirp_sub, ctx_dir_sub->dir_path);
            //         if (sub_res != 0)
            //         {
            //             LOG_WRN("open subdir %s failed, err:%d", ctx_dir_sub->dir_path, sub_res);
            //             continue;
            //         }

            //         while (1)
            //         {
            //             sub_res = fs_readdir(&dirp_sub, &entyr_sub);
            //             if (sub_res != 0 || entyr_sub.name[0] == 0)
            //             {
            //                 break;
            //             }
            //             /* 只抓取文件，忽略子目录，不再继续深入 */
            //             if (entyr_sub.type != FS_DIR_ENTRY_FILE)
            //             {
            //                 continue;
            //             }

            //             LOG_DBG("  [SUB FILE] %s (size = %zu)", entyr_sub.name, entyr_sub.size);

            //             if (file_endswith(entry_sub.name, ".bmp"))
            //             {
            //                 struct ctx_file *ctx_file_sub = k_malloc(sizeof(struct ctx_file));
            //                 if (ctx_file_sub == NULL)
            //                 {
            //                     LOG_WRN("malloc sub file failed, skip %s", entyr_sub.name);
            //                     continue;
            //                 }

            //                 snprintf(ctx_file_sub->file_path, sizeof(ctx_file_sub->file_path),
            //                          "%s/%s", ctx_dir_sub->dir_path, entyr_sub.name);

            //                 sys_dlist_append(&ctx_dir_sub->dlist_file, &ctx_file_sub->file_node);
            //                 ctx_dir_sub->file_num++;
            //             }
            //             else
            //             {
            //                  LOG_WRN("invalid file extension: %s", entry_sub.name);
            //             }
            //         }
            //         fs_closedir(&dirp_sub);
        }
        else /* 解析根目录下的文件 */
        {

            LOG_DBG("[First-level FILE] %s (size = %zu)\n",
                    entry.name, entry.size);

            if (file_endswith(entry.name, ".TXT"))
            {
            }
            else if (file_endswith(entry.name, ".BMP"))
            {
                struct ctx_file *ctx_file_root = k_malloc(sizeof(struct ctx_file));
                if (ctx_file_root == NULL)
                {
                    LOG_WRN("malloc file failed, skip %s", entry.name);
                    continue;
                }

                snprintf(ctx_file_root->file_path, sizeof(ctx_file_root->file_path),
                         "%s/%s", disk_mount_pt, entry.name);

                sys_dlist_append(&ctx_dir_root->dlist_file, &ctx_file_root->file_node);
                ctx_dir_root->file_num++;
            }
            else
            {
                LOG_WRN("invalid file extension: %s", entry.name);
            }
        }
        cnt++;
    }

    LOG_DBG("Root dir scan done, total entries: %u", cnt);

    fs_closedir(&dirp);
    LOG_DBG("Root dir closed");

    fs_unmount(&mp);
    LOG_INF("Disk unmounted");

    return 0;
}

int tf_check_disk(const char *disk_name)
{
    uint64_t memory_size_mb;
    uint32_t block_count;
    uint32_t block_size;

    if (disk_access_ioctl(disk_name, DISK_IOCTL_CTRL_INIT, NULL) != 0)
    {
        LOG_ERR("Storage init ERROR!");
        return -1;
    }

    if (disk_access_ioctl(disk_name, DISK_IOCTL_GET_SECTOR_COUNT, &block_count))
    {
        LOG_ERR("Unable to get sector count");
        return -1;
    }
    LOG_DBG("Block count %u", block_count);

    if (disk_access_ioctl(disk_name, DISK_IOCTL_GET_SECTOR_SIZE, &block_size))
    {
        LOG_ERR("Unable to get sector size");
        return -1;
    }
    LOG_DBG("Sector size %u\n", block_size);

    memory_size_mb = (uint64_t)block_count * block_size;
    LOG_DBG("Memory Size(MB) %u\n", (uint32_t)(memory_size_mb >> 20));

    if (disk_access_ioctl(disk_name,
                          DISK_IOCTL_CTRL_DEINIT, NULL) != 0)
    {
        LOG_ERR("Storage deinit ERROR!");
        return -1;
    }

    return 0;
}
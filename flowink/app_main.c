#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/logging/log.h>
#include <ff.h>
#include <zephyr/fs/fs.h>
#include <zephyr/storage/disk_access.h>
#include <zephyr/sys/dlist.h>
#include <string.h>
#include <stdlib.h>
LOG_MODULE_REGISTER(main);

// #define AUTOMOUNT_NODE DT_NODELABEL(ffs_sd)
// FS_FSTAB_DECLARE_ENTRY(AUTOMOUNT_NODE); /* 声明挂载点，方便后面卸载 */

#define DISK_DRIVE_NAME "SD"
#define DISK_MOUNT_PT "/" DISK_DRIVE_NAME ":"
// #define FS_RET_OK FR_OK
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
    char dir_name[32];

    sys_dnode_t dir_node;
    sys_dlist_t dlist_file;

    uint16_t file_num;
};

struct ctx_file
{
    char file_path[128];
    char file_name[32];

    sys_dnode_t file_node;
};

uint16_t loop_delay;
char *folder_path;
bool loop_subfolder;
char *start_file_name;

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

int main(void)
{

    static const char *disk_pdrv = DISK_DRIVE_NAME;
    static const char *disk_mount_pt = DISK_MOUNT_PT;
    int res;
    uint64_t memory_size_mb;
    uint32_t block_count;
    uint32_t block_size;

    sys_dlist_init(&dlist_dir);

    do
    {
        if (disk_access_ioctl(disk_pdrv,
                              DISK_IOCTL_CTRL_INIT, NULL) != 0)
        {
            LOG_ERR("Storage init ERROR!");
            break;
        }

        if (disk_access_ioctl(disk_pdrv,
                              DISK_IOCTL_GET_SECTOR_COUNT, &block_count))
        {
            LOG_ERR("Unable to get sector count");
            break;
        }
        LOG_INF("Block count %u", block_count);

        if (disk_access_ioctl(disk_pdrv,
                              DISK_IOCTL_GET_SECTOR_SIZE, &block_size))
        {
            LOG_ERR("Unable to get sector size");
            break;
        }
        printk("Sector size %u\n", block_size);

        memory_size_mb = (uint64_t)block_count * block_size;
        printk("Memory Size(MB) %u\n", (uint32_t)(memory_size_mb >> 20));
    } while (0);

    mp.mnt_point = disk_mount_pt;
    res = fs_mount(&mp);

    if (res == 0)
    {
        printk("Disk mounted.\n");
        /* Try to unmount and remount the disk */
        res = fs_unmount(&mp);
        if (res != 0)
        {
            printk("Error unmounting disk\n");
            return res;
        }
        res = fs_mount(&mp);
        if (res != 0)
        {
            printk("Error remounting disk\n");
            return res;
        }
    }
    else
    {
        printk("Error mounting disk.\n");
        return res;
    }

    uint16_t cnt = 0;
    struct fs_dir_t dirp;
    struct fs_dirent entry;
    // struct fs_mount_t *auto_mount_point = &FS_FSTAB_ENTRY(AUTOMOUNT_NODE);

    // fs_dir_t_init(&dirp);
    // res = fs_opendir(&dirp, disk_mount_pt);
    // if (res != 0)
    // {
    //     printk("Error opening root dir [%d]\n", res);
    //     return res;
    // }

    // struct ctx_dir *ctx_dir_root = k_malloc(sizeof(struct ctx_dir));
    // if (ctx_dir_root == NULL)
    // {
    //     LOG_ERR("malloc root dir failed");
    //     return -ENOMEM;
    // }

    // strcpy(ctx_dir_root->dir_path, disk_mount_pt);
    // strcpy(ctx_dir_root->dir_name, "");

    // sys_dlist_init(&ctx_dir_root->dlist_file);
    // ctx_dir_root->file_num = 0;
    // sys_dlist_append(&dlist_dir, &ctx_dir_root->dir_node);

    // /* 仅支持根路径下的额外一层文件夹 */
    // while (1)
    // {
    //     /* readdir 函数会自动偏移，指向 dirp 的下一个 entry */
    //     res = fs_readdir(&dirp, &entry);

    //     if (res || entry.name[0] == 0)
    //     {
    //         break;
    //     }

    //     if (entry.type == FS_DIR_ENTRY_DIR)
    //     {
    //         LOG_DBG("[First-level DIR ] %s\n", entry.name);
    //         struct ctx_dir *ctx_dir_sub = k_malloc(sizeof(struct ctx_dir));
    //         if (ctx_dir_sub == NULL)
    //         {
    //             LOG_WRN("malloc sub dir failed, skip %s", entry.name);
    //             continue;
    //         }

    //         snprintf(ctx_dir_sub->dir_path, sizeof(ctx_dir_sub->dir_path),
    //                  "%s/%s", disk_mount_pt, entry.name);
    //         strncpy(ctx_dir_sub->dir_name, entry.name, sizeof(ctx_dir_sub->dir_name) - 1);
    //         ctx_dir_sub->dir_name[sizeof(ctx_dir_sub->dir_name) - 1] = '\0';

    //         sys_dlist_init(&ctx_dir_sub->dlist_file);
    //         ctx_dir_sub->file_num = 0;

    //         sys_dlist_append(&dlist_dir, &ctx_dir_sub->dir_node);

    //         /* 解析额外一层文件夹：读取一级子文件夹内部所有文件 */
    //         struct fs_dir_t dirp_sub;
    //         struct fs_dirent entyr_sub;
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

    //             if (file_endswith(entry.name, ".bmp"))
    //             {
    //                 struct ctx_file *ctx_file_sub = k_malloc(sizeof(struct ctx_file));
    //                 if (ctx_file_sub == NULL)
    //                 {
    //                     LOG_WRN("malloc sub file failed, skip %s", entyr_sub.name);
    //                     continue;
    //                 }

    //                 snprintf(ctx_file_sub->file_path, sizeof(ctx_file_sub->file_path),
    //                          "%s/%s", ctx_dir_sub->dir_path, entyr_sub.name);
    //                 strncpy(ctx_file_sub->file_name, entyr_sub.name, sizeof(ctx_file_sub->file_name) - 1);
    //                 ctx_file_sub->file_name[sizeof(ctx_file_sub->file_name) - 1] = '\0';

    //                 sys_dlist_append(&ctx_dir_sub->dlist_file, &ctx_file_sub->file_node);
    //                 ctx_dir_sub->file_num++;
    //             }
    //             else
    //             {
    //                 LOG_WRN("invaild file extension");
    //             }
    //         }
    //         fs_closedir(&dirp_sub);
    //     }
    //     else /* 解析根目录下的文件 */
    //     {

    //         LOG_DBG("[First-level FILE] %s (size = %zu)\n",
    //                 entry.name, entry.size);

    //         if (file_endswith(entry.name, ".txt"))
    //         {
    //             char file_path[128];
    //             snprintf(file_path, sizeof(file_path), "%s/%s", disk_mount_pt, entry.name);

    //             struct fs_file_t fd;
    //             fs_file_t_init(&fd);

    //             res = fs_open(&fd, file_path, FS_O_READ);
    //             if (res != 0)
    //             {
    //                 LOG_WRN("Failed to open config file %s, err:%d", file_path, res);
    //                 continue;
    //             }

    //             /* 读取配置文件到缓冲区 */
    //             char buf[512];
    //             ssize_t bytes_read = fs_read(&fd, buf, sizeof(buf) - 1);
    //             if (bytes_read <= 0)
    //             {
    //                 LOG_WRN("Failed to read config file %s", file_path);
    //                 fs_close(&fd);
    //                 continue;
    //             }
    //             buf[bytes_read] = '\0';
    //             fs_close(&fd);

    //             /* 逐行解析 key=value */
    //             char *line = buf;
    //             char *next_line;

    //             while (line != NULL && *line != '\0')
    //             {
    //                 /* 查找当前行结尾 */
    //                 char *end = strchr(line, '\n');
    //                 if (end != NULL)
    //                 {
    //                     *end = '\0';
    //                     next_line = end + 1;
    //                 }
    //                 else
    //                 {
    //                     next_line = NULL;
    //                 }

    //                 /* 去除 Windows 换行符 \r */
    //                 size_t len = strlen(line);
    //                 if (len > 0 && line[len - 1] == '\r')
    //                 {
    //                     line[len - 1] = '\0';
    //                 }

    //                 /* 跳过空行和注释行 */
    //                 if (line[0] != '\0' && line[0] != '#')
    //                 {
    //                     char *eq = strchr(line, '=');
    //                     if (eq != NULL)
    //                     {
    //                         *eq = '\0';
    //                         char *key = line;
    //                         char *value = eq + 1;

    //                         if (strcmp(key, "loop_delay") == 0)
    //                         {
    //                             loop_delay = (uint16_t)atoi(value);
    //                         }
    //                         else if (strcmp(key, "folder_path") == 0)
    //                         {
    //                             if (folder_path != NULL)
    //                             {
    //                                 k_free(folder_path);
    //                             }
    //                             if (strlen(value) > 0)
    //                             {
    //                                 folder_path = k_malloc(strlen(value) + 1);
    //                                 if (folder_path != NULL)
    //                                 {
    //                                     strcpy(folder_path, value);
    //                                 }
    //                             }
    //                             else
    //                             {
    //                                 folder_path = NULL;
    //                             }
    //                         }
    //                         else if (strcmp(key, "loop_subfolder") == 0)
    //                         {
    //                             loop_subfolder = (atoi(value) == 1);
    //                         }
    //                         else if (strcmp(key, "start_file_name") == 0)
    //                         {
    //                             if (start_file_name != NULL)
    //                             {
    //                                 k_free(start_file_name);
    //                             }
    //                             if (strlen(value) > 0)
    //                             {
    //                                 start_file_name = k_malloc(strlen(value) + 1);
    //                                 if (start_file_name != NULL)
    //                                 {
    //                                     strcpy(start_file_name, value);
    //                                 }
    //                             }
    //                             else
    //                             {
    //                                 start_file_name = NULL;
    //                             }
    //                         }
    //                     }
    //                 }
    //                 line = next_line;
    //             }

    //             LOG_INF("Config loaded: delay=%u, folder=%s, sub_loop=%d, start=%s",
    //                     loop_delay,
    //                     folder_path ? folder_path : "(root)",
    //                     loop_subfolder,
    //                     start_file_name ? start_file_name : "(none)");
    //         }
    //         else if (file_endswith(entry.name, ".bmp"))
    //         {
    //             struct ctx_file *ctx_file_root = k_malloc(sizeof(struct ctx_file));
    //             if (ctx_file_root == NULL)
    //             {
    //                 LOG_WRN("malloc file failed, skip %s", entry.name);
    //                 continue;
    //             }

    //             snprintf(ctx_file_root->file_path, sizeof(ctx_file_root->file_path),
    //                      "%s/%s", disk_mount_pt, entry.name);
    //             strncpy(ctx_file_root->file_name, entry.name, sizeof(ctx_file_root->file_name) - 1);
    //             ctx_file_root->file_name[sizeof(ctx_file_root->file_name) - 1] = '\0';

    //             sys_dlist_append(&ctx_dir_root->dlist_file, &ctx_file_root->file_node);
    //             ctx_dir_root->file_num++;
    //         }
    //         else
    //         {
    //             LOG_WRN("invaild file extension");
    //         }
    //     }
    //     cnt++;
    // }

    // fs_closedir(&dirp);
    // LOG_INF("Root dir scan done, total entries: %u", cnt);

    fs_unmount(&mp);
    LOG_INF("Disk unmounted");

    return 0;
}
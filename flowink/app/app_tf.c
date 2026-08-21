#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/logging/log.h>
#include <ff.h>
#include <zephyr/fs/fs.h>
#include <zephyr/storage/disk_access.h>
#include <zephyr/multi_heap/shared_multi_heap.h>
#include <zephyr/sys/dlist.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>

LOG_MODULE_REGISTER(app_tf, LOG_LEVEL_DBG);

#define DISK_DRIVE_NAME "SD"
#define DISK_MOUNT_PT "/" DISK_DRIVE_NAME ":"
#define CONFIG_FILE_PATH DISK_MOUNT_PT "/config.txt" // "/SD:/config.txt"
#define BMP_SUFFIX ".bmp"

static FATFS fat_fs;

/**
 * TF 模块规范：
 * 1. 芯片整个执行流程不会释放、修改、删除链表节点（链表节点顺序和fat系统读取顺序一致）
 *
 */
static struct fs_mount_t mp = {
    .type = FS_FATFS,
    .fs_data = &fat_fs,
};

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

struct config_file_info
{
    char *start_file_path;      /* 轮播开始文件 */
    uint32_t carousel_interval; /* 轮播间隔 */
    uint16_t config_file_size;  /* 配置文件大小 */
    bool loop_play;             /* 文件夹内循环播放，默认开启 */
};

/* 文件夹全部放到一个 list 里面，根目录是头节点，每个 node 有一个 dlist_file*/
static sys_dlist_t dlist_dir;
struct config_file_info config_info = {
    .loop_play = true,
};

/**
 * @brief 判断文件类型
 * @param str 原始字符串，内部转为小写再和后缀进行比较
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

    const char *s = str + (str_len - suf_len);
    for (size_t i = 0; i < suf_len; i++)
    {
        char c1 = (char)tolower((unsigned char)s[i]);
        char c2 = (char)tolower((unsigned char)suffix[i]);
        if (c1 != c2)
        {
            return false;
        }
    }
    return true;
}

/**
 * @brief 为指定目录节点，添加一个file条目到dlist_file链表
 * @param dir_ctx 归属的目录上下文
 * @param full_file_path 文件完整路径
 * @return 0成功，-1内存分配失败
 */
static int ctx_file_append(struct ctx_dir *dir_ctx, const char *full_file_path)
{
    struct ctx_file *ctx_file = k_malloc(sizeof(struct ctx_file));
    if (ctx_file == NULL)
    {
        LOG_WRN("k_malloc ctx_file failed, skip %s", full_file_path);
        return -1;
    }

    snprintf(ctx_file->file_path, sizeof(ctx_file->file_path), "%s", full_file_path);
    sys_dlist_append(&dir_ctx->dlist_file, &ctx_file->file_node);
    dir_ctx->file_num++;
    return 0;
}

/**
 * @brief 检查磁盘状态，读取磁盘信息
 * @param disk_name 磁盘名
 * @return 0成功
 */
static int tf_check_disk(const char *disk_name)
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

    /* 有些磁盘不能重复初始化*/
    if (disk_access_ioctl(disk_name, DISK_IOCTL_CTRL_DEINIT, NULL) != 0)
    {
        LOG_ERR("Storage deinit ERROR!");
        return 0;
    }
}

/**
 * @brief 扫描子文件夹，把内部合法bmp文件挂载到ctx_dir的dlist_file链表
 * @param sub_dir_ctx 子目录上下文
 * @return 0成功
 */
static int scan_sub_dir(struct ctx_dir *sub_dir_ctx)
{
    int ret;
    struct fs_dir_t dirp_sub;
    struct fs_dirent entry_sub;

    fs_dir_t_init(&dirp_sub);
    ret = fs_opendir(&dirp_sub, sub_dir_ctx->dir_path);
    if (ret != 0)
    {
        LOG_WRN("open subdir %s failed, err:%d", sub_dir_ctx->dir_path, ret);
        return -1;
    }

    while (1)
    {
        ret = fs_readdir(&dirp_sub, &entry_sub);
        if (ret || entry_sub.name[0] == 0)
        {
            break;
        }

        /* 忽略子目录，不再继续深入 */
        if (entry_sub.type != FS_DIR_ENTRY_FILE || (!file_endswith(entry_sub.name, BMP_SUFFIX)))
        {
            continue;
        }

        char full_path[128];
        snprintf(full_path, sizeof(full_path), "%s/%s", sub_dir_ctx->dir_path, entry_sub.name);
        LOG_DBG("[SUB FILE] %s (size = %zu)", entry_sub.name, entry_sub.size);

        ctx_file_append(sub_dir_ctx, full_path);
    }

    fs_closedir(&dirp_sub);
    return 0;
}

/**
 * @brief 扫描根目录，填充全局dlist_dir链表
 * @param root_dir_ctx 根目录的ctx_dir指针
 * @return 0成功
 */
static int scan_root_dir(struct ctx_dir *root_dir_ctx)
{
    int ret;
    struct fs_dir_t dirp;
    struct fs_dirent entry;

    fs_dir_t_init(&dirp);
    ret = fs_opendir(&dirp, DISK_MOUNT_PT);
    if (ret != 0)
    {
        LOG_ERR("Error opening root dir [%d]", ret);
        return -1;
    }

    while (1)
    {
        ret = fs_readdir(&dirp, &entry);

        /* 读取结束 */
        if (ret || entry.name[0] == 0)
        {
            break;
        }

        if (entry.type == FS_DIR_ENTRY_DIR) /* 根目录下的文件夹 */
        {
            LOG_DBG("[TOP DIR] %s", entry.name);
            struct ctx_dir *ctx_dir_sub = k_malloc(sizeof(struct ctx_dir));
            if (ctx_dir_sub == NULL)
            {
                LOG_WRN("malloc sub dir failed, skip %s", entry.name);
                continue;
            }

            snprintf(ctx_dir_sub->dir_path, sizeof(ctx_dir_sub->dir_path),
                     "%s/%s", DISK_MOUNT_PT, entry.name);
            sys_dlist_init(&ctx_dir_sub->dlist_file);
            ctx_dir_sub->file_num = 0;
            sys_dlist_append(&dlist_dir, &ctx_dir_sub->dir_node);

            /* 扫描子目录下的文件 */
            scan_sub_dir(ctx_dir_sub);
        }
        else /* 根目录下的文件 */
        {
            LOG_DBG("[TOP FILE] %s (size = %zu)", entry.name, entry.size);

            if (file_endswith(entry.name, ".txt"))
            {
                config_info.config_file_size = entry.size;
                continue;
            }

            if (!file_endswith(entry.name, BMP_SUFFIX))
            {
                continue;
            }

            char full_path[128];
            snprintf(full_path, sizeof(full_path), "%s/%s", DISK_MOUNT_PT, entry.name);
            ctx_file_append(root_dir_ctx, full_path);
        }
    }

    fs_closedir(&dirp);
    return 0;
}

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

    LOG_DBG("mount disk done");

    sys_dlist_init(&dlist_dir);

    struct ctx_dir *ctx_dir_root = k_malloc(sizeof(struct ctx_dir));
    if (ctx_dir_root == NULL)
    {
        LOG_ERR("k_malloc root dir failed");
        fs_unmount(&mp);
        return;
    }

    snprintf(ctx_dir_root->dir_path, sizeof(ctx_dir_root->dir_path), "%s", DISK_MOUNT_PT);
    sys_dlist_init(&ctx_dir_root->dlist_file);
    ctx_dir_root->file_num = 0;
    sys_dlist_append(&dlist_dir, &ctx_dir_root->dir_node);

    scan_root_dir(ctx_dir_root);
}

void tf_deinit(void)
{
    int ret = fs_unmount(&mp);
    if (ret != 0)
    {
        LOG_ERR("Error unmounting disk, err:%d\n", ret);
    }

    LOG_DBG("umount disk done");
}

/**
 * @brief 读取配置文件，注意：该文件的格式有严格要求，无需做过多校验；且避免系统发生崩溃即可，接受完全读取不到有效信息的情况
 * 具体崩溃的点可能有：
 *
 * @param info
 */
void tf_read_config_file(struct config_file_info *info)
{
    int ret = 0;
    struct fs_file_t fd;
    char *file_buf = NULL;
    size_t file_size = 0;
    fs_file_t_init(&fd);

    file_size = info->config_file_size;

    /* 2. pasram分配，多留1字节放字符串结束符 */
    file_buf = (char *)shared_multi_heap_alloc(SMH_REG_ATTR_EXTERNAL, file_size + 1);
    if (file_buf == NULL)
    {
        LOG_WRN("pasram malloc for config file failed, size:%ld", (long)file_size + 1);
        return;
    }

    /* 3. 打开文件读取全部内容到pasram */
    ret = fs_open(&fd, CONFIG_FILE_PATH, FS_O_READ);
    if (ret != 0)
    {
        LOG_WRN("fs_open config file err:%d", ret);
        shared_multi_heap_free(file_buf);
        return;
    }

    ret = fs_read(&fd, file_buf, file_size);
    if (ret != (int)file_size)
    {
        LOG_WRN("fs_read config file err:%d", ret);
        shared_multi_heap_free(file_buf);

        if (ret >= 0)
        {
            fs_close(&fd);
        }
        return;
    }

    fs_close(&fd);
    file_buf[file_size] = '\0'; /* 补字符串结束符 */

    /* ========== 在整块内存上扫描解析 ========== */
    char *p = file_buf;
    while (*p != '\0')
    {
        // 1. 跳过注释行
        if (*p == '#')
        {
            while (*p != '\0' && *p != '\r' && *p != '\n')
                p++;
            while (*p == '\r' || *p == '\n')
                p++;
            continue;
        }

        // 2. 临时截断当前行
        char *line_end = p;
        while (*line_end != '\0' && *line_end != '\r' && *line_end != '\n')
            line_end++;

        char save_ch = *line_end;
        *line_end = '\0';

        // 3. 解析 key="value"
        char *eq_pos = strchr(p, '=');
        if (eq_pos != NULL)
        {
            *eq_pos = '\0';
            char *key = p;
            char *value = eq_pos + 1;

            char *quote_start = strchr(value, '"');
            char *quote_end = (quote_start != NULL) ? strchr(quote_start + 1, '"') : NULL;
            if (quote_start == NULL || quote_end == NULL)
            {
                *line_end = save_ch;
                p = line_end;
                while (*p == '\r' || *p == '\n')
                    p++;
                continue;
            }
            *quote_end = '\0';
            value = quote_start + 1;

            if (strcmp(key, "loop_interval") == 0)
            {
                info->carousel_interval = atoi(value);
            }
            else if (strcmp(key, "loop_subfolder") == 0)
            {
                if (strcmp(value, "1") == 0)
                    info->loop_play = true;
                else if (strcmp(value, "0") == 0)
                    info->loop_play = false;
                else
                {
                    LOG_WRN("loop_subfolder invalid val '%s', keep default true", value);
                    info->loop_play = true;
                }
            }
            else if (strcmp(key, "start_file_name") == 0)
            {
                if (info->start_file_path != NULL)
                {
                    k_free(info->start_file_path);
                    info->start_file_path = NULL;
                }

                if (value[0] != '\0')
                {
                    size_t path_size = strlen(DISK_MOUNT_PT) + 1 + strlen(value) + 1;
                    char *full_path = k_malloc(path_size);
                    if (full_path != NULL)
                        snprintf(full_path, path_size, "%s/%s", DISK_MOUNT_PT, value);
                    else
                        LOG_WRN("malloc start_file_path fail");

                    info->start_file_path = full_path;
                }
                else
                {
                    info->start_file_path = NULL;
                }
            }
        }

        // 4. 恢复并跳到下一行
        *line_end = save_ch;
        p = line_end;
        while (*p == '\r' || *p == '\n')
            p++;
    }

    /* 解析完成释放pasram内存 */
    shared_multi_heap_free(file_buf);
    return;
}

static int tf_find_bmp_file(const char *file_path)
{
    sys_dnode_t node_root = sys_dlist_peek_head(&dlist_dir);
    
    while (1)
    {

    }
}

/* 重新上电之后读取只会读取配置文件并且刷图 */
/* 传入当前文件路径，读取下一张bmp文件到bmp_buf*/
void tf_read_bmp_file(const char *file_path_current, uint8_t *bmp_buf)
{
    /* 遍历所有文件夹链表的所有文件链表节点的所有文件，找出 current 文件 */
    /* */
    while (1)
    {

    }
}
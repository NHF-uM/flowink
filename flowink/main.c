#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/logging/log.h>
#include <ff.h>
#include <zephyr/fs/fs.h>
#include <zephyr/storage/disk_access.h>
#include "dlist.h"

LOG_MODULE_REGISTER(main);

#define AUTOMOUNT_NODE DT_NODELABEL(ffs_sd)
FS_FSTAB_DECLARE_ENTRY(AUTOMOUNT_NODE); /* 声明挂载点，方便后面卸载 */

static sys_dlist_t dlist_dir;

struct pic_dir_entry
{
    char dir_path[128];
    char dir_name[32];

    sys_dnode_t dir_node;
    sys_dlist_t dlist_file;

    uint16_t file_num;
};

struct pic_file_entry
{
    char file_path[128];
    char file_name[32];

    sys_dnode_t file_node;
};

int main(void)
{
    sys_dlist_init(&dlist_dir);


    int res;
    uint16_t cnt = 0;
    struct fs_dir_t dirp;
    struct fs_dirent entry;
    struct fs_mount_t *auto_mount_point = &FS_FSTAB_ENTRY(AUTOMOUNT_NODE);

    fs_dir_t_init(&dirp);
    res = fs_opendir(&dirp, "/SD:");
    if (res)
    {
        printk("Error opening root dir [%d]\n", res);
        return res;
    }

    struct pic_dir_entry *pic_dir_entry_root =  k_malloc(sizeof(struct pic_dir_entry));
    
    sys_dlist_append(&dlist_dir, pic_dir_entry_root->dir_node);
    while (1)
    {
        /* readdir函数会自动偏移，指向dirp的下一个dir */
        res = fs_readdir(&dirp, &entry);

        if (res || entry.name[0] == 0)
        {
            break;
        }

        if (entry.type == FS_DIR_ENTRY_DIR)
        {
            printk("[DIR ] %s\n", entry.name);
        }
        else
        {
            printk("[FILE] %s (size = %zu)\n",
                   entry.name, entry.size);
        }
        cnt++;
    }

    fs_closedir(&dirp);
    if (res == 0)
    {
        res = cnt;
    }
    printk("cnt: %d", cnt);

    res = fs_unmount(auto_mount_point);
    if (res != 0)
    {
        LOG_ERR("Failed to unmount SD filesystem, err:%d", res);
    }
    else
    {
        LOG_INF("SD filesystem unmount success");
    }

    return 0;
}

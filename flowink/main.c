#define LOG_LEVEL 4
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(main);

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/input/input.h>
#include <ff.h>
#include <zephyr/fs/fs.h>
#include <zephyr/storage/disk_access.h>

/*
 * cp -rfpv ../zephyr/samples/subsys/fs/fs_sample/ . 
 * cp -rfpv ../zephyr/samples/subsys/fs/fatfs_fstab/ .
 */

#define AUTOMOUNT_NODE DT_NODELABEL(ffs_sd)


int main(void)
{
    while (1) {

    }
    return 0;
}


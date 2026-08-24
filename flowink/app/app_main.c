#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(main, LOG_LEVEL_DBG);

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

    tf_test_ls_dlist();
    
    char *file_name = tf_read_config_file();
    LOG_INF("start file_name: %s", file_name);

    char *file_path = tf_find_first_bmp();
    LOG_INF("first file_path: %s", file_path);

    char *next_bmp_path = tf_find_next_bmp(file_path);
    LOG_INF("next file_path: %s", next_bmp_path);

    tf_deinit();
    return 0;
}

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(main, LOG_LEVEL_DBG);

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
    tf_init(false);

    tf_test_ls_dlist();
    
    tf_read_config_file();

    LOG_INF("loop_play: %d", tf_get_loop_play() ? 1 : 0);
    LOG_INF("carousel_interval: %d", tf_get_carousel_interval());

    char *file_path = tf_find_first_bmp();
    if (file_path == NULL)
    {
        LOG_ERR("find first bmp failed");
        tf_deinit();
    }
    else
    {
        LOG_INF("first file_path: %s", file_path);
    }

    char *next_bmp_path = tf_find_next_bmp("/SD:/eeb.bmp");
    if (next_bmp_path == NULL)
    {
        LOG_ERR("find next bmp failed after eeb.bmp");
        tf_deinit();
    }

    next_bmp_path = tf_find_next_bmp("/SD:/1b5.bmp");
    if (next_bmp_path == NULL)
    {
        LOG_ERR("find next bmp failed after 1b5.bmp");
        tf_deinit();
    }

    tf_test_change_loop_play(false);

    next_bmp_path = tf_find_next_bmp("/SD:/1b5.bmp");
    if (next_bmp_path == NULL)
    {
        LOG_ERR("find next bmp failed after 1b5.bmp");
        tf_deinit();
    }

    next_bmp_path = tf_find_next_bmp("/SD:/bbb/ed19f0.bmp");
    if (next_bmp_path == NULL)
    {
        LOG_ERR("find next bmp failed after ed19f0.bmp");
        tf_deinit();
    }

    next_bmp_path = tf_find_next_bmp("/SD:/ccc/988.bmp");

    tf_deinit();
    return 0;
}

//   一、建议的 SD 卡文件结构（覆盖全部边界）

//   /SD:
//   ├── config.txt                      # loop_subfolder=0 时用于关循环测试
//   ├── root0.bmp                       # 根目录文件（根目录链表）
//   ├── root1.bmp
//   │
//   ├── DirA/
//   │   ├── a1.bmp
//   │   ├── a2.bmp
//   │   └── a3.bmp                      # 测"链表中间/末尾"顺序
//   │
//   ├── DirB/
//   │   └── b1.bmp                      # 单文件目录：测跳目录
//   │
//   ├── DirEmpty/                       # 空目录：必须被跳过
//   │   (无任何文件)
//   │
//   └── DirIgnore/
//       └── note.txt                    # 非 .bmp 文件：必须被过滤
//       └── ignore.bmp                  # 双结尾节点

// 以及根目录没有任何文件的情况，测试 find_first_bmp()！
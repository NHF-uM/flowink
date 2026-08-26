#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/logging/log.h>
#include "pwr_manage.h"
#include "tf.h"
#include "nv.h"
#include "show_picture.h"

LOG_MODULE_REGISTER(app_carousel, LOG_LEVEL_DBG);

void app_carousel_timer_wakeup_run(bool tf_init_fail)
{
    /* 不需要处理失败情况，timeer_wakeup 唤醒几乎不可能 malloc 失败 */
    show_pic_malloc(NULL);

    if (tf_init_fail)
    {
        show_pic_buildin_bmp();
        goto end;
    }

    char *path = nv_read_path();
    if (path != NULL)
    {
        /* 上次休眠之后又删除了图片 或者 最后一张图片 返回 NULL，复用 path 为 next_path */
        char *next_path = tf_find_next_bmp(path);

        if (next_path != NULL)
        {
            show_pic_tf_bmp(next_path);
        }
        else
        {
            show_pic_tf_bmp(NULL);
        }

        nv_free_path(path);
    }
    else
    {
        /* nv读取失败或者路径无效，播放 start */
        show_pic_tf_bmp(NULL);
    }

end:
    show_pic_free();

    /* 立即休眠 */
    pwr_set_sleep_timer_wakeup(tf_get_carousel_interval());
    pwr_enter_sleep();
}
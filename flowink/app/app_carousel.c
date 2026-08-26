// #include <zephyr/kernel.h>
// #include <zephyr/device.h>
// #include <zephyr/drivers/gpio.h>
// #include <zephyr/multi_heap/shared_multi_heap.h>
// #include <zephyr/drivers/retained_mem.h>
// #include <zephyr/logging/log.h>
// #include "pwr_manage.h"
// #include "led.h"
// #include "btn.h"
// #include "rgb_strip.h"
// #include "svc_bmp.h"
// #include "epd.h"
// #include "net.h"
// #include "tf.h"
// #include "nv.h"

// LOG_MODULE_REGISTER(app_carousel, LOG_LEVEL_DBG);

// void app_carousel_timer_wakeup_run(bool tf_init_fail)
// {
//     uint8_t *data_bmp = shared_multi_heap_alloc(SMH_REG_ATTR_EXTERNAL, CONFIG_BMP_ORIGINAL_SIZE);
//     uint8_t *data_epd = shared_multi_heap_alloc(SMH_REG_ATTR_EXTERNAL, CONFIG_EPD_SEND_BUF_SIZE);
//     if (data_bmp == NULL || data_epd == NULL)
//     {
//         LOG_ERR("Failed to allocate memory for data_epd");
//         if (data_bmp)
//             shared_multi_heap_free(data_bmp);
//         if (data_epd)
//             shared_multi_heap_free(data_epd);
//         return -1;
//     }
//     memset(data_bmp, 0xFF, CONFIG_BMP_ORIGINAL_SIZE);
//     memset(data_epd, 0xFF, CONFIG_EPD_SEND_BUF_SIZE);

//     if (tf_init_fail)
//     {

//         goto end;
//     }

//     char *path = nv_read_path();
//     if (path != NULL)
//     {
//         /* 每次唤醒都会刷新链表，只有在上次休眠之后又删除了图片才会返回 NULL，复用 path 为 next_path */
//         char *next_path = tf_find_next_bmp(path); /* real_path 可能不存在*/

//         if (next_path != NULL)
//         {
//             play_bmp(next_path);
//         }
//         else
//         {
//             play_bmp(tf_get_start_file_path());
//         }

//         nv_free_path(path);
//     }
//     else
//     {
//         /* nv读取失败或者路径无效，播放 start */
//         play_bmp(tf_get_start_file_path());
//     }

// end:
//     shared_multi_heap_free(data_bmp);
//     shared_multi_heap_free(data_epd);

//     /* 立即休眠 */
//     pwr_set_sleep_timer_wakeup(tf_get_carousel_interval());
//     pwr_enter_sleep();
// }
#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/logging/log.h>
#include "pwr_manage.h"
#include "rgb_strip.h"
#include "net.h"
#include "epd.h"
#include "tf.h"
#include "show_picture.h"

LOG_MODULE_REGISTER(app_mode, LOG_LEVEL_DBG);

static void app_enter_sleep(uint32_t sleep_time_s)
{
    tf_deinit();
    epd_sleep();
    k_sleep(K_MSEC(100));
    pwr_set_sleep_timer_wakeup(sleep_time_s);
    pwr_enter_sleep(true);
}

void app_mode_basic_handler(bool is_tf_init_failure)
{
    LOG_DBG("Basic mode selected");

    int ret = show_pic_malloc(NULL);
    if (ret != 0)
    {
        LOG_ERR("no enough heap, enter deep sleep");
        tf_deinit();
        epd_sleep();
        k_sleep(K_MSEC(100));
        pwr_enter_sleep(false);
    }

    if (is_tf_init_failure)
    {
        show_pic_buildin_bmp();
    }
    else
    {
        show_pic_tf_bmp(NULL);
    }

    show_pic_free();

    LOG_DBG("Basic mode finished, enter deep sleep after 2 seconds");
    app_enter_sleep(tf_get_carousel_interval());
}

void app_mode_server_handler(void)
{
    LOG_DBG("Server mode selected");

    uint8_t *bmp_buf = NULL;
    int ret = show_pic_malloc(&bmp_buf);
    if (ret != 0)
    {
        LOG_ERR("no enough heap, enter deep sleep");
        tf_deinit();
        epd_sleep();
        k_sleep(K_MSEC(100));
        pwr_enter_sleep(false);
    }

    wifi_init();
    http_server_start();
    http_set_revc_buf(bmp_buf);

    /* 死等就好了，用户如果不想再上传就进深休再唤醒 */
    LOG_DBG("Waiting for data upload to complete...");
    k_sem_take(&sem_http_data_uping, K_FOREVER);

    show_pic_server_bmp();
    show_pic_free();

    LOG_DBG("Server mode finished, stopping HTTP server and deinitializing Wi-Fi...");
    http_server_stop();
    wifi_deinit1();

    LOG_DBG("Server mode finished, enter deep sleep after 2 seconds");
    app_enter_sleep(24UL * 3600);
}

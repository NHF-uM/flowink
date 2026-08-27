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

static void timer_enter_sleep_fn(struct k_timer *timer);
K_TIMER_DEFINE(timer_enter_sleep, timer_enter_sleep_fn, NULL);

static void timer_enter_sleep_fn(struct k_timer *timer)
{
    epd_sleep();
    tf_deinit();
    pwr_enter_sleep(true);
}

void app_mode_basic_handler(bool is_tf_init_failure)
{
    LOG_DBG("Basic mode selected");

    k_timer_stop(&timer_enter_sleep);

    int ret = show_pic_malloc(NULL);
    if (ret != 0)
    {
        LOG_ERR("no enough heap, enter deep sleep");
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

    LOG_DBG("Basic mode finished, enter deep sleep after 5 minutes");

    /* 阻塞 3 秒让日志稳定输出 */
    k_sleep(K_SECONDS(3));
    pwr_set_sleep_timer_wakeup(tf_get_carousel_interval());
    k_timer_start(&timer_enter_sleep, K_MINUTES(5), K_NO_WAIT);
}

void app_mode_server_handler(void)
{
    LOG_DBG("Server mode selected");

    k_timer_stop(&timer_enter_sleep);

    uint8_t *bmp_buf = NULL;
    int ret = show_pic_malloc(&bmp_buf);
    if (ret != 0)
    {
        LOG_ERR("no enough heap, enter deep sleep");
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

    LOG_DBG("Server mode finished, enter deep sleep after 5 minutes");

    /* 阻塞 3 秒让日志稳定输出 */
    k_sleep(K_SECONDS(3));
    pwr_set_sleep_timer_wakeup(24 * 3600);
    k_timer_start(&timer_enter_sleep, K_MINUTES(5), K_NO_WAIT);
}

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/logging/log.h>
#include "btn.h"
#include "led.h"
#include "epd.h"
#include "tf.h"
#include "rgb_strip.h"
#include "pwr_manage.h"
#include "app_mode.h"
#include "app_carousel.h"

#include "svc_bmp.h"
#include "epd.h"

LOG_MODULE_REGISTER(main, LOG_LEVEL_DBG);

static void enter_sleep_cb(void)
{
    LOG_DBG("Enter sleep mode");
    tf_deinit();
    epd_sleep();
    k_sleep(K_MSEC(100));
    pwr_enter_sleep(false);
}

static void clear_panel_cb(void)
{
    LOG_DBG("Clear panel");
    led_set(led_mode, false, K_NO_WAIT);
    led_set(led_pwr, true, K_MSEC(500));
    epd_show_color(EPD_COLOR_WHITE);
    led_set(led_pwr, true, K_FOREVER);
    led_set(led_mode, true, K_MSEC(200));
}

/* 深度休眠会清空所有 RAM 数据和 PSRAM 数据*/
int main(void)
{
    pwr_init();
    led_init();
    led_set(led_pwr, true, K_FOREVER);
    epd_init();
    int ret = tf_init(false);
    if (ret)
    {
        LOG_WRN("Failed to initialize TF, related functions will be disabled.");
    }

    /* 两个唤醒模式只能运行一个，且运行完就会进入深度休眠，唤醒后从 main 函数重新开始运行*/
    wakeup_source_t wake_cause = pwr_get_wakeup_cause();
    if (wake_cause == WAKEUP_TIMER)
    {
        app_carousel_timer_wakeup_run(ret);
    }
    else
    {
        btn_init();
        led_set(led_mode, true, K_MSEC(200));

        while (1)
        {
            /* 这里要开启阻塞前清除标志位，因为软件定时器有自己的线程，所以事件会随时触发 */
            uint32_t flags = k_event_wait(&btn_mode_event, BTN_BIT_MODE_ALL | BTN_BIT_WAKEUP_ALL, true, K_FOREVER);
            if (flags & BTN_BIT_WAKEUP_CLICKED)
            {
                enter_sleep_cb();
            }
            else if (flags & BTN_BIT_WAKEUP_LONG_PRESSED)
            {
                clear_panel_cb();
            }
            else if (flags & BTN_BIT_MODE_SELECTED)
            {
                led_set(led_mode, false, K_NO_WAIT);

                if (flags & BTN_BIT_MODE_BASIC)
                {
                    app_mode_basic_handler(ret);
                }
                else if (flags & BTN_BIT_MODE_SERVER)
                {
                    app_mode_server_handler();
                }
            }
            else
            {
                if (flags & BTN_BIT_MODE_BASIC)
                {
                    LOG_DBG("Basic mode led invoked");
                    led_set(led_mode, true, K_MSEC(200));
                }
                else if (flags & BTN_BIT_MODE_SERVER)
                {
                    LOG_DBG("Server mode led invoked");
                    led_set(led_mode, true, K_MSEC(1000));
                }
            }
        }
    }

    return 0;
}

void rgb_strip_thread_entry(void)
{
    while (1)
    {
        rgb_strip_on(BLUE);
        k_sleep(K_SECONDS(1));
        rgb_strip_on(RED);
        k_sleep(K_SECONDS(1));
    }
}

K_THREAD_DEFINE(rgb_strip_thread, 1024, rgb_strip_thread_entry, NULL, NULL, NULL,
                7, 0, 0);

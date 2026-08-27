#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/logging/log.h>
#include "btn.h"
#include "led.h"
#include "epd.h"
#include "rgb_strip.h"
#include "pwr_manage.h"
#include "app_mode.h"

#include "svc_bmp.h"
#include "epd.h"

LOG_MODULE_REGISTER(main, LOG_LEVEL_DBG);

/* 提供两个exe：一个用来乱序并且排序号，一个用来展示和拖拽图片，最后排序号  */
/* 深度休眠会清空所有 RAM 数据和 PSRAM 数据*/
int main(void)
{
    app_mode_server_handler();
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

#define LOG_LEVEL 4
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(main);

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include "pwr_manage.h"
#include "rgb_strip.h"

int main(void)
{
    k_sleep(K_SECONDS(30));
    pwr_set_sleep_timer_wakeup(60);

    uint8_t color = BLUE;
    if (pwr_get_wakeup_cause() == WAKEUP_TIMER) 
    {
        color = RED;
    } 
    else 
    {
        color = GREEN;
    }
    pwr_enter_sleep();
    while (1) {
        rgb_strip_on(color);
        k_sleep(K_MSEC(1000));
        rgb_strip_off();
        k_sleep(K_MSEC(1000));
    }
    return 0;
}


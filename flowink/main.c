#define LOG_LEVEL 4
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(main);

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include "pwr_manage.h"
#include "rgb_strip.h"
#include <esp_sleep.h>
int main(void)
{
    pwr_get_wakeup_cause();

    k_sleep(K_SECONDS(30));
    esp_sleep_enable_timer_wakeup(30 * 1000 * 1000);
    pwr_enter_sleep();

    while (1) {

    }
    return 0;
}


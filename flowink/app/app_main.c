#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/logging/log.h>
#include "pwr_manage.h"
#include "led.h"
#include "app_btn.h"
#include "rgb_strip.h"
LOG_MODULE_REGISTER(main);

int main(void)
{
    pwr_init();

    wakeup_source_t wake_cause = pwr_get_wakeup_cause();

    if (wake_cause == WAKEUP_TIMER)
    {
    }
    else
    {
        btn_init();

        while (1)
        {
            k_event_wait(&btn_mode_event, BTN_BIT_MODE_BASIC | BTN_BIT_MODE_SERVER | BTN_BIT_MODE_SELECTED, true, K_FOREVER);
            uint32_t flags = k_event_test(&btn_mode_event, BTN_BIT_MODE_BASIC | BTN_BIT_MODE_SERVER | BTN_BIT_MODE_SELECTED);
            if (flags & BTN_BIT_MODE_SELECTED)
            {
                if (flags & BTN_BIT_MODE_BASIC)
                {
                    LOG_DBG("Basic mode selected");
                }
                else if (flags & BTN_BIT_MODE_SERVER)
                {
                    LOG_DBG("Server mode selected");
                }
            }
            else 
            {
                if (flags & BTN_BIT_MODE_BASIC)
                {
                    led_set();
                }
                else if (flags & BTN_BIT_MODE_SERVER)
                {
                    led_set();
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

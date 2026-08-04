#define LOG_LEVEL 4
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(main);

#include <zephyr/kernel.h>
#include <zephyr/device.h>

#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/spi.h>
#include <zephyr/input/input.h>
#include "rgb_strip.h"
#include "epd.h"

uint8_t color = 0;
int main(void)
{

    epd_init();
    epd_fill_color(EPD_COLOR_BLUE));
    epd_sleep();
    
    k_sleep(K_MINUTES(3));
    epd_fill_color(EPD_COLOR_YELLOW);

    // extern void epd_reset(void);
    // epd_reset();
    // epd_fill_color(EPD_COLOR_YELLOW);

    

    while (1)
    {
        rgb_strip_on(color);
        k_sleep(K_MSEC(1000));
        rgb_strip_off();
        k_sleep(K_MSEC(1000));
    }
    
    return 0;
}

static void button_input_cb(struct input_event *evt, void *user_data)
{
    if (evt->sync == 0)
    {
        return;
    }

    color = (color + 1) % 3;
}

INPUT_CALLBACK_DEFINE(NULL, button_input_cb, NULL);
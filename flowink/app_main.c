#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/logging/log.h>
#include "rgb_strip.h"
#include "epd.h"

LOG_MODULE_REGISTER(main);

int main(void)
{
    epd_init();
    epd_sleep();
    
    k_sleep(K_MINUTES(3));

    epd_reset();
    epd_fill_color(EPD_COLOR_WHITE);
    epd_sleep();
    
    return 0;
}

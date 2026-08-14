#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/logging/log.h>
#include "rgb_strip.h"
#include "epd.h"
#include "test_bmp.h"

LOG_MODULE_REGISTER(main);

int main(void)
{
    test_bmp();   
    return 0;
}

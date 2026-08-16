#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/logging/log.h>
#include "rgb_strip.h"
#include "epd.h"
#include "test.h"

LOG_MODULE_REGISTER(main);

int main(void)
{
	test_bmp();
    return 0;
}

// void rgb_strip_thread_entry(void)
// {
// 	while (1)
// 	{
// 		rgb_strip_on(BLUE);
// 		k_sleep(K_SECONDS(1));
// 		rgb_strip_on(RED);
// 		k_sleep(K_SECONDS(1));
// 	}
// }

// K_THREAD_DEFINE(rgb_strip_thread, 1024, rgb_strip_thread_entry, NULL, NULL, NULL,
// 				7, 0, 0);

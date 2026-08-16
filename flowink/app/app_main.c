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
	int ret;
	pwr_init();

	led_init();
	led_set(led_pwr, true, K_NO_WAIT);
	led_set(led_mode, true, K_MSEC(500));
	k_sleep(K_MSEC(3000));
	led_set(led_mode, false, K_NO_WAIT);

	btn_init();

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

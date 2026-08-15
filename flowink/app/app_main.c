#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/logging/log.h>
// #include "pwr_manage.h"
#include "led.h"
#include "app_btn.h"
LOG_MODULE_REGISTER(main);

int main(void)
{
	int ret;
	// pwr_init();

	led_init();
	led_set(led_pwr, true, K_NO_WAIT);
	led_set(led_mode, true, K_MSEC(500));
	k_sleep(K_MSEC(3000));
	led_set(led_mode, false, K_NO_WAIT);

	btn_init();
	// led_set(&ctx_pwr, true, K_NO_WAIT);
	// wakeup_source_t cause = pwr_get_wakeup_cause();
	// if (cause == WAKEIP_TIMER)
	// {

	// }
	// else
	// {
	// 	/* 等待按键 */
	// 	k_sleep(K_FOREVER);
	// }

	return 0;
}
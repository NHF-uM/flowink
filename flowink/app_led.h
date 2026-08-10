#ifndef _APP_LED_H_
#define _APP_LED_H_

#include <zephyr/drivers/gpio.h>

struct led_ctx
{
	const struct gpio_dt_spec *gpio;
	struct k_timer timer;
	bool blinky_en;
};

extern struct led_ctx led_pwr;
extern struct led_ctx led_mode;

void app_led_init(void);
void app_led_set(struct led_ctx *ctx, bool enable, k_timeout_t period);

#endif

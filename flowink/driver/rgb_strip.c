#include "rgb_strip.h"
#include <zephyr/drivers/led_strip.h>
#include <zephyr/device.h>

#define LED_STRIP_NODE DT_ALIAS(led_strip)

#define RGB(_r, _g, _b) { .r = (_r), .g = (_g), .b = (_b) }

static const struct device *const strip = DEVICE_DT_GET(LED_STRIP_NODE);

static const struct led_rgb colors[] = {
	RGB(CONFIG_LED_STRIP_BRIGHTNESS, 0x00, 0x00), /* red */
	RGB(0x00, CONFIG_LED_STRIP_BRIGHTNESS, 0x00), /* green */
	RGB(0x00, 0x00, CONFIG_LED_STRIP_BRIGHTNESS), /* blue */
};

void rgb_strip_on(uint8_t color)
{
	struct led_rgb color_on = colors[color];
    led_strip_update_rgb(strip, &color_on, (size_t)1);
}

void rgb_strip_off(void)
{
	struct led_rgb colors_off = RGB(0x00, 0x00, 0x00);
    led_strip_update_rgb(strip, &colors_off, (size_t)1);
}

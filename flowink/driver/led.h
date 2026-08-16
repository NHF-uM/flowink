#ifndef _APP_LED_H_
#define _APP_LED_H_

#include <zephyr/kernel.h>

/* 不透明类型，暴露的必须是指针而非实体 */
struct led_ctx;

extern struct led_ctx *const led_pwr;
extern struct led_ctx *const led_mode;

void led_init(void);

/**
 * @brief LED控制：关闭 / 常亮 / 闪烁
 * @param ctx struct led_ctx *
 * @param enable true打开LED；false关闭LED
 * @param period 闪烁半周期：K_NO_WAIT 为常亮，处于关闭状态时该参数不生效
 */
void led_set(struct led_ctx *ctx, bool enable, k_timeout_t period);

#endif

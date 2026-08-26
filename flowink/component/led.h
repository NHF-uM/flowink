#ifndef _LED_H_
#define _LED_H_

#include <zephyr/kernel.h>

/**
 * 不透明指针：.h 里只写 struct led; 前向声明，字段藏到 .c 里，编译器直接拒绝外部读字段，代价是字段不可见后无法栈分配（大小和偏移量不知道），也无法被嵌套继承
 * 真实用武之地是跨二进制库：库做成 .so / .dll / .a，应用代码以 binary 形式连进来，库的 struct 字段在不同版本可能改动，把字段藏起来是 ABI 兼容性的硬需求。自家应用代码（一份工程一起编译）几乎不用。
 * 
 * ```c
 * 
 * struct led_ctx;
 * 
 * extern struct led_ctx *const led_pwr;
 * extern struct led_ctx *const led_mode;
 * 
 * ```
 * 
 */

struct led_ctx
{
    const struct gpio_dt_spec *gpio;    /* private: 不得外部直接修改（不能通过强制手段保护，因为后续的特性需要暴露完整的定义） */
    struct k_timer timer;               /* private: 不得外部直接修改（不能通过强制手段保护，因为后续的特性需要暴露完整的定义） */
    bool blinky_en;                     /* private: 不得外部直接修改（不能通过强制手段保护，因为后续的特性需要暴露完整的定义） */
};

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

#define LOG_LEVEL 4
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(main);

#include <zephyr/kernel.h>
#include <zephyr/device.h>

int main(void)
{
    /* 等待ic初始化 */
    k_sleep(K_SECONDS(5));
    while (1) {

    }
    return 0;
}


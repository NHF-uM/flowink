#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(pwr_m);

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/sys/poweroff.h>
#include <esp_sleep.h>

inline void pwr_enter_sleep(void)
{
    // 每次深度休眠前必须重配唤醒源
    sys_poweroff();
}


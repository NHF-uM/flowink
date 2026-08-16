#ifndef _PWR_MANAGER_H_
#define _PWR_MANAGER_H_


typedef enum {
    WAKEUP_TIMER,
    WAKEUP_IO,
    WAKEUP_UNKNOWN = 0xff
} wakeup_source_t;

/**
 * @brief 配置唤醒引脚，更新记录唤醒源
 * @param  无
 */
void pwr_init(void);

/**
 * @brief 获取深度休眠唤醒原因
 * @param  无
 */
wakeup_source_t pwr_get_wakeup_cause(void);

/**
 * @brief 设置深度休眠唤醒时间
 * @param time_s 单位/秒，范围300~86400
 */
void pwr_set_sleep_timer_wakeup(int time_s);

/**
 * @brief 进入深度休眠
 * @param  无
 */
void pwr_enter_sleep(void);

#endif

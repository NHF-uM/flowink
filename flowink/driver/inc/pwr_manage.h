#ifndef _PWR_MANAGER_H_
#define _PWR_MANAGER_H_


typedef enum {
    WAKEUP_TIMER,
    WAKEUP_IO,
    WAKEUP_UNKNOWN = 0xff
} wakeup_source_t;

wakeup_source_t pwr_get_wakeup_cause(void);
void pwr_set_sleep_timer_wakeup(int time_s);
void pwr_enter_sleep(void);

#endif

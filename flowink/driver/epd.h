#ifndef _EPD_H_
#define _EPD_H_

#include <zephyr/types.h>

#define EPD_COLOR_BLACK   0x0   
#define EPD_COLOR_WHITE   0x1   
#define EPD_COLOR_YELLOW  0x2   
#define EPD_COLOR_RED     0x3   
#define EPD_COLOR_BLUE    0x5   
#define EPD_COLOR_GREEN   0x6   

#define EPD_7IN3E_WIDTH     (800)
#define EPD_7IN3E_HEIGHT    (480)
#define EPD_DATA_SIZE       (192000)

/**
 * @brief 复位屏幕驱动芯片
 * @param  无
 */
void epd_reset(void);

/**
 * @brief 上电初始化，检查设备 ready ，配置屏幕参数
 * @param  无
 */
void epd_init(void);

/**
 * @brief 发送单色像素数据并刷图，会等待 busy 线释放
 * @param color
 */
void epd_show_color(uint8_t color);

/**
 * @brief 发送 Image 像素数据并刷图，会等待 busy 线释放
 * @param Image 
 */
void epd_show_image(uint8_t *Image);

/**
 * @brief 进入休眠模式，再次唤醒需要调用 epd_reset()
 * @param  无
 */
void epd_sleep(void);

#endif

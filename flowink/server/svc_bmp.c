#include "svc_bmp.h"
#include <string.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(svc_bmp, LOG_LEVEL_DBG);

#define BMP_CHECK_PTR(ptr)                 \
    do                                     \
    {                                      \
        if (!(ptr))                        \
        {                                  \
            LOG_WRN("invalid addr param"); \
            return -1;                     \
        }                                  \
    } while (0)

/**
 * @brief 24BMP 的 R/G/B888 转为 EPD 支持的六色
 * @param r RGB-R
 * @param g RGB-G
 * @param b RGB-B
 * @return EPD 支持的颜色值
 */
static inline uint8_t rgb_to_epd_color(uint8_t r, uint8_t g, uint8_t b)
{
    if (b == 0 && g == 0 && r == 0)
        return 0; /* Black   */
    if (b == 255 && g == 255 && r == 255)
        return 1; /* White   */
    if (b == 0 && g == 255 && r == 255)
        return 2; /* Yellow  */
    if (b == 0 && g == 0 && r == 255)
        return 3; /* Red     */
    if (b == 255 && g == 0 && r == 0)
        return 5; /* Blue    */
    if (b == 0 && g == 255 && r == 0)
        return 6; /* Green   */
    return 1;     /* Default White */
}

/**
 * @brief 向 epd_buf 写入单个像素（E6 全彩 7in3E 采用半字节打包）
 * 注意：未对 tx ty 做边界检查，这里需要保证传入的 bmp 数据大小正确
 * @param epd_buf 待写入缓存
 * @param tx 横坐标
 * @param ty 纵坐标
 * @param panel_w 屏幕宽度
 * @param color 颜色像素
 */
static inline void epd_set_pixel(uint8_t *epd_buf, uint32_t tx, uint32_t ty,
                                 uint32_t panel_w, uint8_t color)
{
    uint32_t addr = tx / 2 + ty * (panel_w / 2);

    if (tx & 1)
        epd_buf[addr] = (epd_buf[addr] & 0xF0) | color;
    else
        epd_buf[addr] = (epd_buf[addr] & 0x0F) | (color << 4);
}

int bmp_decode_to_epd(const uint8_t *bmp_buf, uint8_t *epd_buf, bool rotate_180)
{
    BMP_CHECK_PTR(bmp_buf);
    BMP_CHECK_PTR(epd_buf);

    const bmp_file_header_t *fh = (const bmp_file_header_t *)bmp_buf;
    const bmp_info_header_t *ih = (const bmp_info_header_t *)(bmp_buf + sizeof(bmp_file_header_t));

    if (fh->bType != 0x4D42 || ih->biBitCount != 24)
    {
        LOG_ERR("invalid bmp header, type=0x%04X, bitcount=%d", fh->bType, ih->biBitCount);
        return -1;
    }

    const uint32_t w = ih->biWidth;
    const uint32_t h = ih->biHeight;
    const uint32_t pw = CONFIG_EPD_PANEL_WIDTH;
    const uint32_t ph = CONFIG_EPD_PANEL_HEIGHT;
    const bool is_portrait = (w == 480 && h == 800);

    if (!is_portrait && !(w == 800 && h == 480))
    {
        LOG_ERR("unsupported resolution %ux%u", w, h);
        return -1;
    }

    const uint32_t row_stride = (w * 3 + 3) & ~3;
    const uint8_t *pixel = bmp_buf + fh->bOffset;

    for (uint32_t y = 0; y < h; y++)
    {
        const uint8_t *row = pixel + y * row_stride;
        const uint32_t Y = rotate_180 ? y : (h - 1 - y);

        for (uint32_t x = 0; x < w; x++)
        {
            const uint8_t b = row[x * 3 + 0];
            const uint8_t g = row[x * 3 + 1];
            const uint8_t r = row[x * 3 + 2];
            const uint8_t color = rgb_to_epd_color(r, g, b);

            uint32_t tx, ty;
            if (is_portrait)
            {
                /* 经验证，这里要对竖图做顺时针90度旋转，映射到物理尺寸 */
                tx = y;
                ty = x;

                if (rotate_180)
                {
                    tx = pw - tx - 1;
                    ty = ph - ty - 1;
                }
            }
            else
            {
                tx = rotate_180 ? (w - x - 1) : x;
                ty = Y;
            }

            epd_set_pixel(epd_buf, tx, ty, is_portrait ? pw : w, color);
        }
    }
    
    return 0;
}
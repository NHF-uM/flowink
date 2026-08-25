#include "svc_bmp.h"
#include <string.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(svc_bmp, LOG_LEVEL_DBG);

#define BMP_CHECK_PTR(ptr)            \
    do                                \
    {                                 \
        if (!(ptr))                   \
        {                             \
            LOG_WRN("invalid param"); \
            return -1;                \
        }                             \
    } while (0)

int bmp_decode_to_epd(const uint8_t *bmp_buf, uint8_t *epd_buf, bool rotate_180)
{
    BMP_CHECK_PTR(bmp_buf);
    BMP_CHECK_PTR(epd_buf);

    bmp_file_header_t *fh = (bmp_file_header_t *)bmp_buf;
    bmp_info_header_t *ih = (bmp_info_header_t *)(bmp_buf + sizeof(bmp_file_header_t));

    LOG_DBG("BMP header: type=0x%04X, bitcount=%d, offset=%u", fh->bType, ih->biBitCount, fh->bOffset);

    /* 校验 BMP 文件头部信息 */
    if (fh->bType != 0x4D42 || ih->biBitCount != 24)
    {
        LOG_ERR("invalid bmp header, type=0x%04X, bitcount=%d, expect BM&24bpp", fh->bType, ih->biBitCount);
        return -1;
    }

    bool res_ok = (ih->biWidth == 800 && ih->biHeight == 480) || (ih->biWidth == 480 && ih->biHeight == 800);
    if (!res_ok)
    {
        LOG_ERR("unsupported resolution, w=%u h=%u, only support 800x480/480x800", ih->biWidth, ih->biHeight);
        return -1;
    }

    uint32_t width = ih->biWidth;
    uint32_t height = ih->biHeight;
    uint32_t width_byte = width / 2;            /* 2 像素/字节 */
    uint32_t row_stride = (width * 3 + 3) & ~3; /* BMP 每行 4 字节对齐 */
    const uint8_t *pixel = bmp_buf + fh->bOffset;

    for (uint32_t y = 0; y < height; y++)
    {
        const uint8_t *row = pixel + y * row_stride;
        uint32_t Y = rotate_180 ? y : (height - 1 - y); /* 合成后的垂直变换 */

        for (uint32_t x = 0; x < width; x++)
        {
            /* 24位 BMP （一个像素占三个字节）=> 6色像素数据（一个像素占一个字节） */
            uint8_t b = row[x * 3 + 0];
            uint8_t g = row[x * 3 + 1];
            uint8_t r = row[x * 3 + 2];

            uint8_t color;
            if (b == 0 && g == 0 && r == 0)
                color = 0; // Black
            else if (b == 255 && g == 255 && r == 255)
                color = 1; // White
            else if (b == 0 && g == 255 && r == 255)
                color = 2; // Yellow
            else if (b == 0 && g == 0 && r == 255)
                color = 3; // Red
            else if (b == 255 && g == 0 && r == 0)
                color = 5; // Blue
            else if (b == 0 && g == 255 && r == 0)
                color = 6; // Green
            else
                color = 1; // 默认白色

            /* 合成后的水平变换 */
            uint32_t X = rotate_180 ? (width - x - 1) : x;
            uint32_t addr = X / 2 + Y * width_byte;

            if (X % 2 == 0)
                epd_buf[addr] = (epd_buf[addr] & 0x0F) | (color << 4); // 偶数列 -> 高 nibble
            else
                epd_buf[addr] = (epd_buf[addr] & 0xF0) | color; // 奇数列 -> 低 nibble
        }
    }

    LOG_DBG("bmp decode to epd finish, rotate_180=%d", rotate_180);
    return 0;
}
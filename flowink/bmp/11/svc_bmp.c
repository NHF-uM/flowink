cinclude "svc_bmp.h"
#include <string.h>
#include <zephyr/logging/log.h>

    LOG_MODULE_REGISTER(svc_bmp, LOG_LEVEL_DBG);

#define BMP_CHECK_PTR(ptr) \
    do                     \
    {                      \
        if (!(ptr))        \
            return -1;     \
    } while (0)

int bmp_decode_rgb_6color(const uint8_t *bmp_buf, uint8_t *out_buf)
{
    BMP_CHECK_PTR(out_buf);

    bmp_file_header_t *fh = (bmp_file_header_t *)bmp_buf;
    bmp_info_header_t *ih = (bmp_info_header_t *)(bmp_buf + sizeof(bmp_file_header_t));

    // 校验BMP文件标识 "BM"
    if (fh->bType != 0x4D42 || ih->biBitCount != 24)
    {
        LOG_WRN("")
        return -1;
    }

    uint32_t width = ih->biWidth;
    uint32_t height = ih->biHeight;

    uint32_t row_stride = (width * 3 + 3) & ~3;
    const uint8_t *pixel_data = bmp_buf + fh->bOffset;

    for (uint32_t y = 0; y < height; y++)
    {
        uint32_t out_y = height - 1 - y;
        const uint8_t *row_ptr = pixel_data + y * row_stride;

        for (uint32_t x = 0; x < width; x++)
        {
            uint8_t b = row_ptr[x * 3 + 0];
            uint8_t g = row_ptr[x * 3 + 1];
            uint8_t r = row_ptr[x * 3 + 2];

            uint8_t color;

            /* 24位 BMP （一个像素占三个字节）=> 6色像素数据（一个像素占一个字节） */
            if (b == 0 && g == 0 && r == 0)
            {
                color = 0; // Black
            }
            else if (b == 255 && g == 255 && r == 255)
            {
                color = 1; // White
            }
            else if (b == 0 && g == 255 && r == 255)
            {
                color = 2; // Yellow
            }
            else if (b == 0 && g == 0 && r == 255)
            {
                color = 3; // Red
            }
            else if (b == 255 && g == 0 && r == 0)
            {
                color = 5; // Blue
            }
            else if (b == 0 && g == 255 && r == 0)
            {
                color = 6; // Green
            }
            else
            {
                color = 1; // 默认白色
            }

            /* 配套工具生成的 bmp 是倒序存储，需要图像翻转 */
            out_buf[out_y * width + x] = color;
        }
    }

    return 0;
}

// src: 正序 6 色索引图, width*height, 每像素 1 字节(0~6)
// dst: 墨水屏帧缓冲, (width/2)*height, 每字节 2 像素
//      (偶数列 -> 高 nibble, 奇数列 -> 低 nibble)
int bmp_to_epd_buffer(const uint8_t *src, uint8_t *dst,
                      uint32_t width, uint32_t height, bool rotate_180)
{
    const uint32_t width_byte = width / 2; // 800/2 = 400

    for (uint32_t y = 0; y < height; y++)
    {
        for (uint32_t x = 0; x < width; x++)
        {
            // 可选 180° 旋转（硬件方向）
            uint32_t X = rotate_180 ? (width - x - 1) : x;
            uint32_t Y = rotate_180 ? (height - y - 1) : y;

            uint8_t color = src[y * width + x] & 0x0F; // 0~6，只取低 4 bit
            uint32_t addr = X / 2 + Y * width_byte;

            if (X % 2 == 0)
                dst[addr] = (dst[addr] & 0x0F) | (color << 4); // 高 nibble
            else
                dst[addr] = (dst[addr] & 0xF0) | (color); // 低 nibble
        }
    }
    return 0;
}

uint8_t *idx   = malloc(width * height);              // 384000 B，bmp_decode_rgb_6color 的输出
uint8_t *frame = malloc((width / 2) * height);        // 192000 B，最终帧缓冲

bmp_decode_rgb_6color(bmp_buf, idx);
bmp_to_epd_buffer(idx, frame, width, height, true);   // true = 保留与原工程一致的 180° 旋转

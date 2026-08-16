/*****************************************************************************
* | File        :   bmp_decoder.c
* | Author      :   Waveshare team (original), modified for portability
* | Function    :   Memory-based BMP decoder core implementation
* | Info        :
*                Core logic operates purely on memory buffers.
*                No filesystem IO, no display driver dependency.
*----------------
* |	This version:   V3.0
* | Date        :   2026-08-13
* | Info        :
* -----------------------------------------------------------------------------
* V3.0 核心修改点 (2026-08-13):
* // MODIFIED: 移除所有文件IO操作(fopen/fread/fclose)，输入改为内存缓冲区
* // MODIFIED: 移除Paint_SetPixel直接绘制，改为输出到调用方提供的像素缓冲区
* // MODIFIED: 替换自定义UBYTE/UWORD/UDOUBLE为标准stdint类型
* // MODIFIED: 移除ESP专属heap_caps_malloc、esp_log依赖
* // MODIFIED: 新增bmp_read_headers内部辅助函数，统一头解析与合法性校验
* // MODIFIED: 行字节对齐通过stride计算自动跳过填充，无需单独处理
* // MODIFIED: 重命名文件为bmp_decoder.c，符合MIT协议修改要求
*
* 原始版本记录:
* V2.3(2022-07-27):
* 1.Add GUI_ReadBmp_RGB_4Color()
* V2.2(2020-07-08):
* 1.Add GUI_ReadBmp_RGB_7Color()
* V2.1(2019-10-10):
* 1.Add GUI_ReadBmp_4Gray()
* V2.0(2018-11-12):
* 1.Change file name: GUI_BMP.h -> GUI_BMPfile.h
* 2.fix: GUI_ReadBmp()
#
# MIT License - 完整协议见头文件
#
******************************************************************************/

#include "bmp_decoder.h"
#include <string.h>
#include <stdio.h>

// MODIFIED: 新增空指针校验宏
#define BMP_CHECK_PTR(ptr) \
    do                     \
    {                      \
        if (!(ptr))        \
            return -1;     \
    } while (0)

/**
 * @brief 校验并提取 BMP 文件的头部信息
 * @param bmp_buf BMP 缓冲区
 * @param fh 文件头二级指针
 * @param ih 信息头二级指针
 * @return 0成功，-1失败
 */
static int bmp_read_headers(const uint8_t *bmp_buf, const bmp_file_header_t **fh,
                            const bmp_info_header_t **ih)
{
    BMP_CHECK_PTR(bmp_buf);
    BMP_CHECK_PTR(fh);
    BMP_CHECK_PTR(ih);

    *fh = (const bmp_file_header_t *)bmp_buf;
    *ih = (const bmp_info_header_t *)(bmp_buf + sizeof(bmp_file_header_t));

    // 校验BMP文件标识 "BM"
    if ((*fh)->bType != 0x4D42)
    {
        return -1;
    }

    return 0;
}

int bmp_get_info(const uint8_t *bmp_buf, uint32_t buf_len,
                 uint32_t *width, uint32_t *height, uint8_t *bit_count)
{
    const bmp_file_header_t *fh;
    const bmp_info_header_t *ih;

    if (bmp_read_headers(bmp_buf, &fh, &ih) != 0)
    {
        return -1;
    }

    if (width)
        *width = ih->biWidth;
    if (height)
        *height = ih->biHeight;
    if (bit_count)
        *bit_count = ih->biBitCount;

    return 0;
}

// ===================== 1位单色BMP解码 =====================
int bmp_decode_mono(const uint8_t *bmp_buf, uint8_t *out_buf,
                    uint8_t color_0, uint8_t color_1)
{
    const bmp_file_header_t *fh;
    const bmp_info_header_t *ih;

    // MODIFIED: 替换原fopen+fread读头为内存头解析
    if (bmp_read_headers(bmp_buf, &fh, &ih) != 0)
    {
        return -1;
    }
    BMP_CHECK_PTR(out_buf);

    uint32_t width = ih->biWidth;
    uint32_t height = ih->biHeight;

    // MODIFIED: 校验位深，原逻辑在函数内判断
    if (ih->biBitCount != 1)
    {
        return -1;
    }

    // 计算行字节数与4字节对齐步长
    uint32_t row_bytes = (width + 7) / 8;
    uint32_t row_stride = (row_bytes + 3) & ~3;

    // MODIFIED: 调色板指针直接从内存偏移获取，替代fread读调色板
    const bmp_rgb_quad_t *palette =
        (const bmp_rgb_quad_t *)(bmp_buf + sizeof(bmp_file_header_t) + sizeof(bmp_info_header_t));

    // MODIFIED: 根据调色板第一个颜色判断黑白映射，逻辑与原代码完全一致
    // [修改说明] 最终语义：color_0 = 深色(黑, 位=1)输出值，color_1 = 浅色(白, 位=0)输出值。
    // 与颜色索引约定(0=黑,1=白)一致，且与头文件注释保持一致。
    uint8_t c0, c1;
    if (palette[0].rgbBlue == 0xff &&
        palette[0].rgbGreen == 0xff &&
        palette[0].rgbRed == 0xff)
    {
        c0 = color_1;
        c1 = color_0;
    }
    else
    {
        c0 = color_0;
        c1 = color_1;
    }

    // MODIFIED: 像素数据直接从内存偏移读取，替代fseek+fread
    const uint8_t *pixel_data = bmp_buf + fh->bOffset;

    // MODIFIED: 从内存逐行解析，替代逐字节fread
    for (uint32_t y = 0; y < height; y++)
    {
        // BMP底部向上存储：文件第一行对应图像最后一行
        uint32_t out_y = height - 1 - y;
        const uint8_t *row_ptr = pixel_data + y * row_stride;

        for (uint32_t x = 0; x < width; x++)
        {
            uint8_t byte_val = row_ptr[x / 8];
            uint8_t bit_mask = 0x80 >> (x % 8);
            out_buf[out_y * width + x] = (byte_val & bit_mask) ? c1 : c0;
        }
    }

    return 0;
}

// ===================== 4级灰度BMP解码 =====================
int bmp_decode_4gray(const uint8_t *bmp_buf, uint8_t *out_buf)
{
    const bmp_file_header_t *fh;
    const bmp_info_header_t *ih;

    if (bmp_read_headers(bmp_buf, &fh, &ih) != 0)
    {
        return -1;
    }
    BMP_CHECK_PTR(out_buf);

    uint32_t width = ih->biWidth;
    uint32_t height = ih->biHeight;

    if (ih->biBitCount != 4)
    {
        return -1;
    }

    uint32_t row_stride = ((width / 2) + 3) & ~3;
    const uint8_t *pixel_data = bmp_buf + fh->bOffset;

    for (uint32_t y = 0; y < height; y++)
    {
        uint32_t out_y = height - 1 - y;
        const uint8_t *row_ptr = pixel_data + y * row_stride;

        for (uint32_t x = 0; x < width; x++)
        {
            uint8_t byte_val = row_ptr[x / 2];
            // MODIFIED: 高低位顺序与原代码完全一致
            uint8_t nibble = (x % 2 == 0) ? (byte_val >> 4) : (byte_val & 0x0F);
            // MODIFIED: 4bit转2bit灰度，与原逻辑完全一致
            out_buf[out_y * width + x] = nibble >> 2;
        }
    }

    return 0;
}

// ===================== 16级灰度BMP解码 =====================
int bmp_decode_16gray(const uint8_t *bmp_buf, uint8_t *out_buf)
{
    const bmp_file_header_t *fh;
    const bmp_info_header_t *ih;

    if (bmp_read_headers(bmp_buf, &fh, &ih) != 0)
    {
        return -1;
    }
    BMP_CHECK_PTR(out_buf);

    uint32_t width = ih->biWidth;
    uint32_t height = ih->biHeight;

    if (ih->biBitCount != 4)
    {
        return -1;
    }

    // MODIFIED: 从内存读取16色调色板，替代循环fread
    const bmp_rgb_quad_t *palette =
        (const bmp_rgb_quad_t *)(bmp_buf + sizeof(bmp_file_header_t) + sizeof(bmp_info_header_t));

    uint8_t color_map[16];
    for (int i = 0; i < 16; i++)
    {
        // MODIFIED: 灰度映射公式与原代码完全一致
        color_map[i] = (palette[i].rgbRed + 8) / 17;
    }

    uint32_t row_stride = ((width + 1) / 2 + 3) & ~3;
    const uint8_t *pixel_data = bmp_buf + fh->bOffset;

    for (uint32_t y = 0; y < height; y++)
    {
        uint32_t out_y = height - 1 - y;
        const uint8_t *row_ptr = pixel_data + y * row_stride;

        for (uint32_t x = 0; x < width; x++)
        {
            uint8_t byte_val = row_ptr[x / 2];
            uint8_t nibble = (x % 2 == 0) ? (byte_val >> 4) : (byte_val & 0x0F);
            out_buf[out_y * width + x] = color_map[nibble];
        }
    }

    return 0;
}

// ===================== 24位RGB转4色 =====================
int bmp_decode_rgb_4color(const uint8_t *bmp_buf, uint8_t *out_buf)
{
    const bmp_file_header_t *fh;
    const bmp_info_header_t *ih;

    if (bmp_read_headers(bmp_buf, &fh, &ih) != 0)
    {
        return -1;
    }
    BMP_CHECK_PTR(out_buf);

    uint32_t width = ih->biWidth;
    uint32_t height = ih->biHeight;

    if (ih->biBitCount != 24)
    {
        return -1;
    }

    // MODIFIED: 24位BMP行步长，自动包含4字节对齐填充
    uint32_t row_stride = (width * 3 + 3) & ~3;
    const uint8_t *pixel_data = bmp_buf + fh->bOffset;

    for (uint32_t y = 0; y < height; y++)
    {
        uint32_t out_y = height - 1 - y;
        const uint8_t *row_ptr = pixel_data + y * row_stride;

        for (uint32_t x = 0; x < width; x++)
        {
            // MODIFIED: BMP 24位为BGR顺序，与原代码Rdata[0]=B Rdata[1]=G Rdata[2]=R完全对应
            uint8_t b = row_ptr[x * 3 + 0];
            uint8_t g = row_ptr[x * 3 + 1];
            uint8_t r = row_ptr[x * 3 + 2];

            uint8_t color;
            // MODIFIED: 阈值判断逻辑与原4色函数完全一致
            if (b < 128 && g < 128 && r < 128)
            {
                color = 0; // Black
            }
            else if (b > 127 && g > 127 && r > 127)
            {
                color = 1; // White
            }
            else if (b < 128 && g > 127 && r > 127)
            {
                color = 2; // Yellow
            }
            else if (b < 128 && g < 128 && r > 127)
            {
                color = 3; // Red
            }
            else
            {
                color = 1; // 默认白色
            }

            out_buf[out_y * width + x] = color;
        }
        // MODIFIED: 行尾填充字节通过row_stride自动跳过，无需单独循环读取
    }

    return 0;
}

// ===================== 24位RGB转6色 =====================
int bmp_decode_rgb_6color(const uint8_t *bmp_buf, uint8_t *out_buf)
{
    const bmp_file_header_t *fh;
    const bmp_info_header_t *ih;

    if (bmp_read_headers(bmp_buf, &fh, &ih) != 0)
    {
        return -1;
    }
    BMP_CHECK_PTR(out_buf);

    uint32_t width = ih->biWidth;
    uint32_t height = ih->biHeight;

    if (ih->biBitCount != 24)
    {
        return -1;
    }

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
            // MODIFIED: 颜色映射与原6色函数完全一致，保留索引4(橙色)注释
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
                // } else if (b == 0 && g == 128 && r == 255) {
                //     color = 4; // Orange
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

            out_buf[out_y * width + x] = color;
        }
    }

    return 0;
}

// ===================== 24位RGB转7色 =====================
int bmp_decode_rgb_7color(const uint8_t *bmp_buf, uint8_t *out_buf)
{
    const bmp_file_header_t *fh;
    const bmp_info_header_t *ih;

    if (bmp_read_headers(bmp_buf, &fh, &ih) != 0)
    {
        return -1;
    }
    BMP_CHECK_PTR(out_buf);

    uint32_t width = ih->biWidth;
    uint32_t height = ih->biHeight;

    if (ih->biBitCount != 24)
    {
        return -1;
    }

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
            // MODIFIED: 颜色映射与原7色函数完全一致
            if (b == 0 && g == 0 && r == 0)
            {
                color = 0; // Black
            }
            else if (b == 255 && g == 255 && r == 255)
            {
                color = 1; // White
            }
            else if (b == 0 && g == 255 && r == 0)
            {
                color = 2; // Green
            }
            else if (b == 255 && g == 0 && r == 0)
            {
                color = 3; // Blue
            }
            else if (b == 0 && g == 0 && r == 255)
            {
                color = 4; // Red
            }
            else if (b == 0 && g == 255 && r == 255)
            {
                color = 5; // Yellow
            }
            else if (b == 0 && g == 128 && r == 255)
            {
                color = 6; // Orange
            }
            else
            {
                color = 1; // 默认白色
            }

            out_buf[out_y * width + x] = color;
        }
    }

    return 0;
}

// ===================== 可选：文件读取封装 =====================
#if BMP_DECODER_ENABLE_FILE_IO
// MODIFIED: 可选的文件读取封装，兼容原使用习惯
// 负责读文件到内存 -> 调用核心解码 -> 释放内存
int bmp_decode_file_mono(const char *path, uint8_t *out_buf,
                         uint8_t color_0, uint8_t color_1)
{
    FILE *fp = fopen(path, "rb");
    if (!fp)
        return -1;

    fseek(fp, 0, SEEK_END);
    long file_size = ftell(fp);
    fseek(fp, 0, SEEK_SET);

    if (file_size <= 0)
    {
        fclose(fp);
        return -1;
    }

    uint8_t *file_buf = (uint8_t *)malloc(file_size);
    if (!file_buf)
    {
        fclose(fp);
        return -1;
    }

    if (fread(file_buf, 1, file_size, fp) != (size_t)file_size)
    {
        free(file_buf);
        fclose(fp);
        return -1;
    }
    fclose(fp);

    int ret = bmp_decode_mono(file_buf, out_buf, color_0, color_1);
    free(file_buf);
    return ret;
}
#endif
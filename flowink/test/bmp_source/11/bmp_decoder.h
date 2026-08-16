/*****************************************************************************
* | File        :   bmp_decoder.h
* | Author      :   Waveshare team (original), modified for portability
* | Function    :   Memory-based BMP image decoder (no filesystem dependency)
* | Info        :
*                Decode BMP images from preloaded memory buffer.
*                Fully decoupled from filesystem and display driver.
*----------------
* |	This version:   V3.0
* | Date        :   2026-08-13
* | Info        :
* -----------------------------------------------------------------------------
* V3.0 (2026-08-13):
* 1. Refactored to memory-based decoding, completely decoupled from file IO
* 2. Removed dependency on DEV_Config.h, use standard stdint types only
* 3. Removed direct display drawing calls, output raw pixel buffer instead
* 4. Added bmp_get_info() for buffer size pre-allocation
* 5. Added input validity checks to improve robustness
* 6. Renamed file from GUI_BMPfile.h to bmp_decoder.h per MIT license
*
* Original version history:
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
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included in
# all copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS OR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
# THE SOFTWARE.
#
******************************************************************************/
#ifndef __BMP_DECODER_H
#define __BMP_DECODER_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * ====================== 使用说明 ======================
 * 1. 先将完整BMP文件读到内存中（可通过fread、SPIFFS、SD卡、网络等任意方式）
 * 2. 调用 bmp_get_info() 获取图像宽、高、位深，计算输出缓冲区大小 = 宽 × 高
 * 3. 由调用方分配输出缓冲区（1字节对应1个像素）
 * 4. 根据BMP类型调用对应解码函数，像素数据将写入输出缓冲区
 * 5. 调用方自行将像素缓冲区绘制到屏幕
 * 
 * 注意：所有解码函数内部不分配内存，必须由调用方提供足够大的输出缓冲区
 * =====================================================
 */

/* Bitmap file header: 14 bytes */
typedef struct {
    uint16_t bType;         // 文件标识，固定为 0x4D42 ("BM")
    uint32_t bSize;         // 整个BMP文件的大小（字节）
    uint16_t bReserved1;    // 保留字段，必须为0
    uint16_t bReserved2;    // 保留字段，必须为0
    uint32_t bOffset;       // 像素数据相对于文件头的偏移量
} __attribute__((packed)) bmp_file_header_t;

/* Bitmap info header: 40 bytes (标准 BITMAPINFOHEADER) */
typedef struct {
    uint32_t biInfoSize;    // 本结构体大小，固定40
    uint32_t biWidth;       // 图像宽度（像素）
    uint32_t biHeight;      // 图像高度（像素），正数表示底部向上存储
    uint16_t biPlanes;      // 颜色平面数，固定为1
    uint16_t biBitCount;    // 每个像素的位数：1/4/24
    uint32_t biCompression; // 压缩方式，0 = 无压缩
    uint32_t bimpImageSize; // 原始像素数据大小
    uint32_t biXPelsPerMeter; // 水平分辨率
    uint32_t biYPelsPerMeter; // 垂直分辨率
    uint32_t biClrUsed;     // 调色板颜色数
    uint32_t biClrImportant;// 重要颜色数
} __attribute__((packed)) bmp_info_header_t;

/* 调色板颜色项 */
typedef struct {
    uint8_t rgbBlue;
    uint8_t rgbGreen;
    uint8_t rgbRed;
    uint8_t rgbReversed;
} __attribute__((packed)) bmp_rgb_quad_t;

/**
 * @brief 从内存BMP数据中读取图像基本信息
 * 
 * @param bmp_buf   内存中完整BMP文件数据指针
 * @param buf_len   BMP数据缓冲区长度
 * @param width     输出：图像宽度（像素）
 * @param height    输出：图像高度（像素）
 * @param bit_count 输出：每个像素的位数
 * @return int      0成功，-1数据无效
 */
int bmp_get_info(const uint8_t *bmp_buf, uint32_t buf_len,
                 uint32_t *width, uint32_t *height, uint8_t *bit_count);

/**
 * @brief 解码1位单色BMP到1字节/像素的缓冲区
 * 
 * @param bmp_buf   内存中完整BMP文件数据指针
 * @param out_buf   输出像素缓冲区（大小 = width * height）
 * @param color_0   深色(黑)像素的输出像素值（位=1 时输出） [修改说明] 原注释“调色板第0项”与实际语义相反，易导致黑白颠倒，已更正
 * @param color_1   浅色(白)像素的输出像素值（位=0 时输出）
 * @return int      0成功，-1失败
 */
int bmp_decode_mono(const uint8_t *bmp_buf, uint8_t *out_buf,
                    uint8_t color_0, uint8_t color_1);

/**
 * @brief 解码4位BMP为4级灰度（输出0~3）
 * 
 * @param bmp_buf   内存中完整BMP文件数据指针
 * @param out_buf   输出像素缓冲区（大小 = width * height）
 * @return int      0成功，-1失败
 */
int bmp_decode_4gray(const uint8_t *bmp_buf, uint8_t *out_buf);

/**
 * @brief 解码4位BMP为16级灰度（输出0~15）
 * 
 * @param bmp_buf   内存中完整BMP文件数据指针
 * @param out_buf   输出像素缓冲区（大小 = width * height）
 * @return int      0成功，-1失败
 */
int bmp_decode_16gray(const uint8_t *bmp_buf, uint8_t *out_buf);

/**
 * @brief 解码24位RGB BMP为4色索引
 *        颜色索引：0=黑, 1=白, 2=黄, 3=红
 * 
 * @param bmp_buf   内存中完整BMP文件数据指针
 * @param out_buf   输出像素缓冲区（大小 = width * height）
 * @return int      0成功，-1失败
 */
int bmp_decode_rgb_4color(const uint8_t *bmp_buf, uint8_t *out_buf);

/**
 * @brief 解码24位RGB BMP为6色索引
 *        颜色索引：0=黑, 1=白, 2=黄, 3=红, 5=蓝, 6=绿
 * 
 * @param bmp_buf   内存中完整BMP文件数据指针
 * @param out_buf   输出像素缓冲区（大小 = width * height）
 * @return int      0成功，-1失败
 */
int bmp_decode_rgb_6color(const uint8_t *bmp_buf, uint8_t *out_buf);

/**
 * @brief 解码24位RGB BMP为7色索引
 *        颜色索引：0=黑, 1=白, 2=绿, 3=蓝, 4=红, 5=黄, 6=橙
 * 
 * @param bmp_buf   内存中完整BMP文件数据指针
 * @param out_buf   输出像素缓冲区（大小 = width * height）
 * @return int      0成功，-1失败
 */
int bmp_decode_rgb_7color(const uint8_t *bmp_buf, uint8_t *out_buf);

/* 可选：文件读取封装，需要stdio时开启 */
#define BMP_DECODER_ENABLE_FILE_IO 0
#if BMP_DECODER_ENABLE_FILE_IO
int bmp_decode_file_mono(const char *path, uint8_t *out_buf,
                         uint8_t color_0, uint8_t color_1);
#endif

#ifdef __cplusplus
}
#endif

#endif /* __BMP_DECODER_H */
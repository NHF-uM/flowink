#ifndef _SVC_BMP_H_
#define _SVC_BMP_H_

#include <stdint.h>
#include <stdbool.h>

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

/**
 * @brief 把原始 bmp 数据转换为 epd 数据
 * 注意：内部未做边界检查，需要保证传入的 bmp 数据大小正确
 * @param bmp_buf  输入 bmp
 * @param epd_buf  输出到epd
 * @param rotate_180 是否需要旋转180°，满足屏幕的放置需求
 * @return
 */
int bmp_decode_to_epd(const uint8_t *bmp_buf, uint8_t *epd_buf, bool rotate_180);

#endif

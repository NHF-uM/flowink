/******************************************************************************
* | File      	:   gui_paint.h
* | Author      :   Waveshare electronics (original), modified for portability
* | Function    :	2D graphics drawing library for packed pixel framebuffer
* | Info        :
*   Support points, lines, rectangles, circles, characters, strings, numbers
*   Support 1bit / 2bit(4gray) / 4bit multi-scale pixel packing
*----------------
* |	This version:   V3.3
* | Date        :   2026-08-13
* | Info        :
* -----------------------------------------------------------------------------
* V3.3
* 1. 移除DEV_Config.h硬件依赖，基于标准stdint类型
* 2. 移除esp_log依赖，提供可开关日志宏
* 3. 修复文字绘制前景/背景色传参颠倒BUG
* 4. 新增paint_blit_image贴图接口，对接BMP解码器输出
* 5. 全面重构命名为小写下划线snake_case
* 6. 头文件增加完整中文注释，接口参数说明完善
*
* Permission is hereby granted, free of charge, to any person obtaining a copy
* of this software and associated documentation files (the "Software"), to deal
* in the Software without restriction, including without limitation the rights
* to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
* copies of the Software, and to permit persons to whom the Software is
* furnished to do so, subject to the following conditions:
*
* The above copyright notice and this permission notice shall be included in
* all copies or substantial portions of the Software.
*
* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
* IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
* FITNESS OR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
* AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
* LIABILITY WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
* OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
* THE SOFTWARE.
******************************************************************************/
#ifndef __GUI_PAINT_H
#define __GUI_PAINT_H

#include <stdint.h>

/* 基础类型别名，统一库内类型，兼容嵌入式平台 */
typedef uint8_t     u8;
typedef uint16_t    u16;
typedef uint32_t    u32;

/* 字体结构体定义
 * [修改说明] 原V3.3此处仅有前置声明(typedef struct s_font s_font_t; 等)，
 * 导致 gui_paint.c 里对 font->height/font->width/font->table 等成员的访问
 * 因“不完整类型”而无法编译。现补全为完整定义，字段名与 gui_paint.c 的
 * 访问方式(lowercase)保持一致。
 * 注意：与 Waveshare 原版 fonts.h 的 sFONT/cFONT(字段 Width/Height/ASCII_Width 大写)
 * 不同，若复用原版字体数据，需写适配层或改用与本定义一致的字体数据。
 */
typedef struct {
    const u8 *table;   /* 字符点阵数据首地址（每字符 height * ceil(width/8) 字节） */
    u16 width;         /* 字符宽度(像素) */
    u16 height;        /* 字符高度(像素) */
} s_font_t;

/* 中文字库单个字符条目 */
typedef struct {
    u8  index[3];      /* 字符索引：ASCII 用 index[0]，中文(GBK) 用 index[0..2] */
    const u8 *matrix;  /* 字符点阵数据 */
} c_font_char_t;

typedef struct {
    const c_font_char_t *table;  /* 字符表数组 */
    u16 size;                    /* 字符表条目数 */
    u16 width;                   /* 字符宽度(像素) */
    u16 height;                  /* 字符高度(像素) */
    u16 ascii_width;             /* ASCII 字符横向步进宽度 */
} c_font_t;

/**
 * @brief 画布上下文结构体
 * @param buf           帧缓冲区首地址
 * @param width         逻辑画布宽度（经过旋转后的显示宽度）
 * @param height        逻辑画布高度（经过旋转后的显示高度）
 * @param mem_width     物理缓冲区原始宽度
 * @param mem_height    物理缓冲区原始高度
 * @param clear_color   默认清屏颜色
 * @param rotate        画布旋转角度 ROTATE_*_DEG
 * @param mirror        镜像模式 MIRROR_*_MODE
 * @param width_byte    每行占用字节数（由打包缩放scale计算）
 * @param height_byte   纵向占用行数
 * @param scale         像素打包缩放模式：2/4/6/7/16
 */
typedef struct {
    u8  *buf;
    u16 width;
    u16 height;
    u16 mem_width;
    u16 mem_height;
    u16 clear_color;
    u16 rotate;
    u16 mirror;
    u16 width_byte;
    u16 height_byte;
    u8  scale;
} paint_ctx_t;

/* 全局单一画布上下文实例 */
extern paint_ctx_t paint;

/**
 * @brief 画布旋转角度定义
 */
#define ROTATE_0_DEG        0
#define ROTATE_90_DEG       90
#define ROTATE_180_DEG      180
#define ROTATE_270_DEG      270

/**
 * @brief 画布镜像模式枚举
 */
typedef enum {
    MIRROR_NONE_MODE        = 0x00,     // 无镜像
    MIRROR_HORIZONTAL_MODE  = 0x01,     // 水平镜像
    MIRROR_VERTICAL_MODE    = 0x02,     // 垂直镜像
    MIRROR_ORIGIN_MODE      = 0x03      // 水平+垂直同时镜像
} mirror_mode_t;
#define MIRROR_DEFAULT_MODE MIRROR_NONE_MODE

/**
 * @brief 颜色常量定义
 * @note 灰度模式下数值含义由scale打包方式决定
 */
#define WHITE               0xFF
#define BLACK               0x00
#define RED                 BLACK

#define IMAGE_BACKGROUND    WHITE
#define FONT_FOREGROUND     BLACK
#define FONT_BACKGROUND     WHITE

/* 4级灰度专用颜色（scale=4） */
#define GRAY_LEVEL_1        0x03    // 最深灰色
#define GRAY_LEVEL_2        0x02
#define GRAY_LEVEL_3        0x01    // 浅灰色
#define GRAY_LEVEL_4        0x00    // 接近白色

/**
 * @brief 绘制点尺寸枚举
 */
typedef enum {
    DOT_PIXEL_1X1  = 1,
    DOT_PIXEL_2X2,
    DOT_PIXEL_3X3,
    DOT_PIXEL_4X4,
    DOT_PIXEL_5X5,
    DOT_PIXEL_6X6,
    DOT_PIXEL_7X7,
    DOT_PIXEL_8X8
} dot_pixel_size_t;
#define DOT_PIXEL_DEFAULT DOT_PIXEL_1X1

/**
 * @brief 点阵填充样式
 */
typedef enum {
    DOT_FILL_AROUND  = 1,       // 向外填充
    DOT_FILL_RIGHTUP            // 右上起始填充
} dot_fill_style_t;
#define DOT_STYLE_DEFAULT DOT_FILL_AROUND

/**
 * @brief 线条样式：实线/虚线
 */
typedef enum {
    LINE_STYLE_SOLID = 0,
    LINE_STYLE_DOTTED
} line_style_t;

/**
 * @brief 图形填充模式：空心/实心填充
 */
typedef enum {
    DRAW_FILL_EMPTY = 0,        // 空心图形
    DRAW_FILL_FULL              // 实心填充图形
} draw_fill_mode_t;

/**
 * @brief 时间结构体，用于绘制时分秒时间文本
 */
typedef struct {
    u16 year;
    u8  month;
    u8  day;
    u8  hour;
    u8  minute;
    u8  second;
} paint_time_t;
extern paint_time_t s_paint_time;

#ifdef __cplusplus
extern "C" {
#endif

/************************** 画布基础操作接口 ******************************/
/**
 * @brief 初始化画布上下文，绑定帧缓冲区、尺寸、旋转、初始底色
 * @param buf        帧缓存内存指针
 * @param width      物理宽度像素
 * @param height     物理高度像素
 * @param rotate     初始旋转角度 ROTATE_*_DEG
 * @param clear_color 画布初始填充色
 */
void paint_new_image(u8 *buf, u16 width, u16 height, u16 rotate, u16 clear_color);

/**
 * @brief 切换绑定新的帧缓冲区（多缓冲双缓存场景使用）
 * @param buf 新帧缓存地址
 */
void paint_select_image(u8 *buf);

/**
 * @brief 设置画布旋转
 * @param rotate ROTATE_0_DEG / ROTATE_90_DEG / ROTATE_180_DEG / ROTATE_270_DEG
 */
void paint_set_rotate(u16 rotate);

/**
 * @brief 设置画布镜像模式
 * @param mirror 参考 mirror_mode_t
 */
void paint_set_mirroring(u8 mirror);

/**
 * @brief 设置像素打包缩放模式，自动重新计算每行字节宽度
 * @param scale 支持 2 / 4 / 6 / 7 / 16
 * scale=2  -> 1bit黑白；scale=4 -> 2bit四灰度；scale>=6 -> 4bit
 */
void paint_set_scale(u8 scale);

/**
 * @brief 在逻辑坐标上绘制单个像素（自动处理旋转、镜像、边界检测）
 * @param x 逻辑X坐标
 * @param y 逻辑Y坐标
 * @param color 像素颜色
 */
void paint_set_pixel(u16 x, u16 y, u16 color);

/**
 * @brief 全屏填充指定颜色
 * @param color 填充颜色
 */
void paint_clear(u16 color);

/**
 * @brief 矩形区域填充颜色（窗口清屏）
 * @param x_start 起始X
 * @param y_start 起始Y
 * @param x_end   结束X（不包含）
 * @param y_end   结束Y（不包含）
 * @param color   填充颜色
 */
void paint_clear_window(u16 x_start, u16 y_start, u16 x_end, u16 y_end, u16 color); /* [修改说明] 参数名顺序原为(y_end, x_end)，已与实现(gui_paint.c)对齐为(x_end, y_end)，均为u16故不影响ABI */

/************************** 基础图形绘制接口 ******************************/
/**
 * @brief 绘制放大点
 * @param x,y 中心点坐标
 * @param color 颜色
 * @param dot_size 点阵大小 dot_pixel_size_t
 * @param fill_style 填充方式 dot_fill_style_t
 */
void paint_draw_point(u16 x, u16 y, u16 color, dot_pixel_size_t dot_size, dot_fill_style_t fill_style);

/**
 * @brief Bresenham算法绘制直线
 * @param x_start,y_start 起点
 * @param x_end,y_end     终点
 * @param color 线条颜色
 * @param line_width 线宽点阵大小
 * @param line_style 实线/虚线 line_style_t
 */
void paint_draw_line(u16 x_start, u16 y_start, u16 x_end, u16 y_end,
                     u16 color, dot_pixel_size_t line_width, line_style_t line_style);

/**
 * @brief 绘制矩形（空心/实心）
 * @param x_start,y_start 左上角
 * @param x_end,y_end     右下角
 * @param color 边框/填充颜色
 * @param line_width 边框宽度
 * @param fill_mode 空心/实心 draw_fill_mode_t
 */
void paint_draw_rectangle(u16 x_start, u16 y_start, u16 x_end, u16 y_end,
                          u16 color, dot_pixel_size_t line_width, draw_fill_mode_t fill_mode);

/**
 * @brief 中点圆算法绘制圆形
 * @param x_center,y_center 圆心
 * @param radius 半径像素
 * @param color 绘制颜色
 * @param line_width 轮廓宽度
 * @param fill_mode 空心圆环/实心圆
 */
void paint_draw_circle(u16 x_center, u16 y_center, u16 radius,
                       u16 color, dot_pixel_size_t line_width, draw_fill_mode_t fill_mode);

/************************** 文字绘制接口 ********************************/
/**
 * @brief 绘制单个ASCII英文字符
 * @param x,y 左上角坐标
 * @param ch 待绘制字符
 * @param font 英文字体指针 s_font_t
 * @param color_fg 前景文字色
 * @param color_bg 背景填充色
 */
void paint_draw_char(u16 x, u16 y, char ch, s_font_t *font, u16 color_fg, u16 color_bg);

/**
 * @brief 绘制英文字符串，自动换行
 * @param x_start,y_start 起始坐标
 * @param str 字符串指针
 * @param font 英文字体
 * @param color_fg 文字前景色
 * @param color_bg 文字背景色
 */
void paint_draw_string_en(u16 x_start, u16 y_start, const char *str,
                          s_font_t *font, u16 color_fg, u16 color_bg);

/**
 * @brief 绘制中英文混合字符串（支持GBK中文字库）
 * @param x_start,y_start 起始坐标
 * @param str 字符串
 * @param font 中文字体 c_font_t
 * @param color_fg 前景色
 * @param color_bg 背景色
 */
void paint_draw_string_cn(u16 x_start, u16 y_start, const char *str,
                          c_font_t *font, u16 color_fg, u16 color_bg);

/**
 * @brief 绘制整数数字
 * @param x,y 起始坐标
 * @param num 32位整型数字
 * @param font 英文字体
 * @param color_fg 前景色
 * @param color_bg 背景色
 */
void paint_draw_num(u16 x, u16 y, int32_t num, s_font_t *font, u16 color_fg, u16 color_bg);

/**
 * @brief 绘制带指定小数位数浮点数
 * @param x,y 起始坐标
 * @param num 浮点数值
 * @param digit 小数保留位数
 * @param font 字体
 * @param color_fg 前景色
 * @param color_bg 背景色
 */
void paint_draw_num_decimals(u16 x, u16 y, double num, u8 digit,
                             s_font_t *font, u16 color_fg, u16 color_bg);

/**
 * @brief 绘制 HH:MM:SS 时分秒时间
 * @param x_start,y_start 起始坐标
 * @param time 时间结构体指针
 * @param font 字体
 * @param color_fg 前景色
 * @param color_bg 背景色
 */
void paint_draw_time(u16 x_start, u16 y_start, paint_time_t *time,
                     s_font_t *font, u16 color_fg, u16 color_bg);

/************************** 图像贴图接口 ********************************/
/**
 * @brief 直接拷贝完整预打包位图到画布（不经过坐标转换，原始帧缓存覆盖）
 * @param buf 源打包位图缓冲区，必须与当前画布宽度字节、高度一致
 */
void paint_draw_bitmap(const u8 *buf);

/**
 * @brief 贴图接口：逐像素将1字节/像素图像绘制到画布
 * @note 自动支持画布旋转、镜像、边界裁剪，用于BMP解码结果渲染
 * @param x_start,y_start 画布上贴图左上角坐标
 * @param pixel_buf 源图像：1 byte 对应1个像素颜色值
 * @param img_width 源图像宽度
 * @param img_height 源图像高度
 */
void paint_blit_image(u16 x_start, u16 y_start, const u8 *pixel_buf, u16 img_width, u16 img_height);

#ifdef __cplusplus
}
#endif

#endif
/******************************************************************************
 * | File      	:   GUI_Paint.c -> gui_paint.c
 * | Author      :   Waveshare electronics (original), modified for portability
 * | Function    :	2D graphics drawing implementation
 * | Info        :
 *   Achieve drawing: points, lines, rectangles, circles, characters, strings
 *   Achieve packed pixel framebuffer management for e-Paper / LCD
 *----------------
 * |	This version:   V3.3
 * | Date        :   2026-08-13
 * | Info        :
 * -----------------------------------------------------------------------------
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 ******************************************************************************/
#include "gui_paint.h"
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

/* 可配置日志开关，编译时控制输出 */
#define GUI_PAINT_LOG_ENABLE 0
#if GUI_PAINT_LOG_ENABLE
#define GUI_LOG(fmt, ...) printf("[gui_paint] " fmt "\r\n", ##__VA_ARGS__)
#else
#define GUI_LOG(fmt, ...) ((void)0)
#endif

paint_ctx_t paint;

/**
 * @brief 初始化画布上下文
 */
void paint_new_image(u8 *buf, u16 width, u16 height, u16 rotate, u16 clear_color)
{
    paint.buf = buf;

    paint.mem_width = width;
    paint.mem_height = height;
    paint.clear_color = clear_color;
    paint.scale = 2;
    paint.width_byte = (width % 8 == 0) ? (width / 8) : (width / 8 + 1);
    paint.height_byte = height;

    paint.rotate = rotate;
    paint.mirror = MIRROR_NONE_MODE;

    if (rotate == ROTATE_0_DEG || rotate == ROTATE_180_DEG)
    {
        paint.width = width;
        paint.height = height;
    }
    else
    {
        paint.width = height;
        paint.height = width;
    }
}

/**
 * @brief 切换帧缓冲区
 */
void paint_select_image(u8 *buf)
{
    paint.buf = buf;
}

/**
 * @brief 设置画布旋转
 */
void paint_set_rotate(u16 rotate)
{
    if (rotate == ROTATE_0_DEG || rotate == ROTATE_90_DEG || rotate == ROTATE_180_DEG || rotate == ROTATE_270_DEG)
    {
        GUI_LOG("set rotate %d", rotate);
        paint.rotate = rotate;
    }
    else
    {
        GUI_LOG("rotate only support 0/90/180/270");
    }
}

/**
 * @brief 设置镜像模式
 */
void paint_set_mirroring(u8 mirror)
{
    if (mirror == MIRROR_NONE_MODE || mirror == MIRROR_HORIZONTAL_MODE ||
        mirror == MIRROR_VERTICAL_MODE || mirror == MIRROR_ORIGIN_MODE)
    {
        GUI_LOG("mirror h:%s v:%s",
                (mirror & 0x01) ? "on" : "off",
                ((mirror >> 1) & 0x01) ? "on" : "off");
        paint.mirror = mirror;
    }
    else
    {
        GUI_LOG("invalid mirror mode");
    }
}

/**
 * @brief 设置像素打包scale，重新计算行字节宽度
 */
void paint_set_scale(u8 scale)
{
    if (scale == 2)
    {
        paint.scale = scale;
        paint.width_byte = (paint.mem_width % 8 == 0) ? (paint.mem_width / 8) : (paint.mem_width / 8 + 1);
    }
    else if (scale == 4)
    {
        paint.scale = scale;
        paint.width_byte = (paint.mem_width % 4 == 0) ? (paint.mem_width / 4) : (paint.mem_width / 4 + 1);
    }
    else if (scale == 6 || scale == 7 || scale == 16)
    {
        paint.scale = scale;
        paint.width_byte = (paint.mem_width % 2 == 0) ? (paint.mem_width / 2) : (paint.mem_width / 2 + 1);
    }
    else
    {
        GUI_LOG("scale error, support 2,4,6,7,16");
    }
}

/**
 * @brief 设置单个像素（逻辑坐标转换物理坐标）
 */
void paint_set_pixel(u16 x_point, u16 y_point, u16 color)
{
    if (x_point > paint.width || y_point > paint.height)
    {
        GUI_LOG("pixel out of canvas");
        return;
    }
    u16 x, y;
    switch (paint.rotate)
    {
    case ROTATE_0_DEG:
        x = x_point;
        y = y_point;
        break;
    case ROTATE_90_DEG:
        x = paint.mem_width - y_point - 1;
        y = x_point;
        break;
    case ROTATE_180_DEG:
        x = paint.mem_width - x_point - 1;
        y = paint.mem_height - y_point - 1;
        break;
    case ROTATE_270_DEG:
        x = y_point;
        y = paint.mem_height - x_point - 1;
        break;
    default:
        return;
    }

    switch (paint.mirror)
    {
    case MIRROR_NONE_MODE:
        break;
    case MIRROR_HORIZONTAL_MODE:
        x = paint.mem_width - x - 1;
        break;
    case MIRROR_VERTICAL_MODE:
        y = paint.mem_height - y - 1;
        break;
    case MIRROR_ORIGIN_MODE:
        x = paint.mem_width - x - 1;
        y = paint.mem_height - y - 1;
        break;
    default:
        return;
    }

    if (x > paint.mem_width || y > paint.mem_height)
    {
        GUI_LOG("pixel out of physical buffer");
        return;
    }

    if (paint.scale == 2)
    {
        u32 addr = x / 8 + y * paint.width_byte;
        u8 r_data = paint.buf[addr];
        if (color == BLACK)
            paint.buf[addr] = r_data & ~(0x80 >> (x % 8));
        else
            paint.buf[addr] = r_data | (0x80 >> (x % 8));
    }
    else if (paint.scale == 4)
    {
        u32 addr = x / 4 + y * paint.width_byte;
        color = color % 4;
        u8 r_data = paint.buf[addr];
        r_data = r_data & (~(0xC0 >> ((x % 4) * 2)));
        paint.buf[addr] = r_data | ((color << 6) >> ((x % 4) * 2));
    }
    else if (paint.scale == 6 || paint.scale == 7 || paint.scale == 16)
    {
        u32 addr = x / 2 + y * paint.width_byte;
        u8 r_data = paint.buf[addr];
        r_data = r_data & (~(0xF0 >> ((x % 2) * 4)));
        paint.buf[addr] = r_data | ((color << 4) >> ((x % 2) * 4));
    }
}

/**
 * @brief 全屏填充颜色
 */
void paint_clear(u16 color)
{
    if (paint.scale == 2)
    {
        for (u16 y = 0; y < paint.height_byte; y++)
        {
            for (u16 x = 0; x < paint.width_byte; x++)
            {
                u32 addr = x + y * paint.width_byte;
                paint.buf[addr] = color;
            }
        }
    }
    else if (paint.scale == 4)
    {
        for (u16 y = 0; y < paint.height_byte; y++)
        {
            for (u16 x = 0; x < paint.width_byte; x++)
            {
                u32 addr = x + y * paint.width_byte;
                paint.buf[addr] = (color << 6) | (color << 4) | (color << 2) | color;
            }
        }
    }
    else if (paint.scale == 6 || paint.scale == 7 || paint.scale == 16)
    {
        for (u16 y = 0; y < paint.height_byte; y++)
        {
            for (u16 x = 0; x < paint.width_byte; x++)
            {
                u32 addr = x + y * paint.width_byte;
                paint.buf[addr] = (color << 4) | color;
            }
        }
    }
}

/**
 * @brief 矩形区域清屏
 */
void paint_clear_window(u16 x_start, u16 y_start, u16 x_end, u16 y_end, u16 color)
{
    u16 x, y;
    for (y = y_start; y < y_end; y++)
    {
        for (x = x_start; x < x_end; x++)
        {
            paint_set_pixel(x, y, color);
        }
    }
}

/**
 * @brief 绘制放大点
 */
void paint_draw_point(u16 x_point, u16 y_point, u16 color,
                      dot_pixel_size_t dot_size, dot_fill_style_t dot_style)
{
    if (x_point > paint.width || y_point > paint.height)
    {
        GUI_LOG("draw point out of range");
        return;
    }

    int16_t x_dir_num, y_dir_num;
    if (dot_style == DOT_FILL_AROUND)
    {
        for (x_dir_num = 0; x_dir_num < 2 * dot_size - 1; x_dir_num++)
        {
            for (y_dir_num = 0; y_dir_num < 2 * dot_size - 1; y_dir_num++)
            {
                if (x_point + x_dir_num - dot_size < 0 || y_point + y_dir_num - dot_size < 0)
                    break;
                paint_set_pixel(x_point + x_dir_num - dot_size, y_point + y_dir_num - dot_size, color);
            }
        }
    }
    else
    {
        for (x_dir_num = 0; x_dir_num < dot_size; x_dir_num++)
        {
            for (y_dir_num = 0; y_dir_num < dot_size; y_dir_num++)
            {
                paint_set_pixel(x_point + x_dir_num - 1, y_point + y_dir_num - 1, color);
            }
        }
    }
}

/**
 * @brief Bresenham绘制直线
 */
void paint_draw_line(u16 x_start, u16 y_start, u16 x_end, u16 y_end,
                     u16 color, dot_pixel_size_t line_width, line_style_t line_style)
{
    if (x_start > paint.width || y_start > paint.height ||
        x_end > paint.width || y_end > paint.height)
    {
        GUI_LOG("draw line out of range");
        return;
    }

    u16 x_point = x_start;
    u16 y_point = y_start;
    int dx = (int)x_end - (int)x_start >= 0 ? x_end - x_start : x_start - x_end;
    int dy = (int)y_end - (int)y_start <= 0 ? y_end - y_start : y_start - y_end;

    int x_addway = x_start < x_end ? 1 : -1;
    int y_addway = y_start < y_end ? 1 : -1;

    int esp = dx + dy;
    char dotted_len = 0;

    for (;;)
    {
        dotted_len++;
        if (line_style == LINE_STYLE_DOTTED && dotted_len % 3 == 0)
        {
            paint_draw_point(x_point, y_point, IMAGE_BACKGROUND, line_width, DOT_STYLE_DEFAULT);
            dotted_len = 0;
        }
        else
        {
            paint_draw_point(x_point, y_point, color, line_width, DOT_STYLE_DEFAULT);
        }
        if (2 * esp >= dy)
        {
            if (x_point == x_end)
                break;
            esp += dy;
            x_point += x_addway;
        }
        if (2 * esp <= dx)
        {
            if (y_point == y_end)
                break;
            esp += dx;
            y_point += y_addway;
        }
    }
}

/**
 * @brief 绘制矩形
 */
void paint_draw_rectangle(u16 x_start, u16 y_start, u16 x_end, u16 y_end,
                          u16 color, dot_pixel_size_t line_width, draw_fill_mode_t fill_mode)
{
    if (x_start > paint.width || y_start > paint.height ||
        x_end > paint.width || y_end > paint.height)
    {
        GUI_LOG("draw rect out of range");
        return;
    }

    if (fill_mode == DRAW_FILL_FULL)
    {
        u16 y_point;
        for (y_point = y_start; y_point < y_end; y_point++)
        {
            paint_draw_line(x_start, y_point, x_end, y_point, color, line_width, LINE_STYLE_SOLID);
        }
    }
    else
    {
        paint_draw_line(x_start, y_start, x_end, y_start, color, line_width, LINE_STYLE_SOLID);
        paint_draw_line(x_start, y_start, x_start, y_end, color, line_width, LINE_STYLE_SOLID);
        paint_draw_line(x_end, y_end, x_end, y_start, color, line_width, LINE_STYLE_SOLID);
        paint_draw_line(x_end, y_end, x_start, y_end, color, line_width, LINE_STYLE_SOLID);
    }
}

/**
 * @brief 中点圆算法绘制圆形
 */
void paint_draw_circle(u16 x_center, u16 y_center, u16 radius,
                       u16 color, dot_pixel_size_t line_width, draw_fill_mode_t fill_mode)
{
    if (x_center > paint.width || y_center >= paint.height)
    {
        GUI_LOG("draw circle out of range");
        return;
    }

    int16_t x_current = 0;
    int16_t y_current = radius;
    int16_t esp = 3 - (radius << 1);

    int16_t s_count_y;
    if (fill_mode == DRAW_FILL_FULL)
    {
        while (x_current <= y_current)
        {
            for (s_count_y = x_current; s_count_y <= y_current; s_count_y++)
            {
                paint_draw_point(x_center + x_current, y_center + s_count_y, color, DOT_PIXEL_DEFAULT, DOT_STYLE_DEFAULT);
                paint_draw_point(x_center - x_current, y_center + s_count_y, color, DOT_PIXEL_DEFAULT, DOT_STYLE_DEFAULT);
                paint_draw_point(x_center - s_count_y, y_center + x_current, color, DOT_PIXEL_DEFAULT, DOT_STYLE_DEFAULT);
                paint_draw_point(x_center - s_count_y, y_center - x_current, color, DOT_PIXEL_DEFAULT, DOT_STYLE_DEFAULT);
                paint_draw_point(x_center - x_current, y_center - s_count_y, color, DOT_PIXEL_DEFAULT, DOT_STYLE_DEFAULT);
                paint_draw_point(x_center + x_current, y_center - s_count_y, color, DOT_PIXEL_DEFAULT, DOT_STYLE_DEFAULT);
                paint_draw_point(x_center + s_count_y, y_center - x_current, color, DOT_PIXEL_DEFAULT, DOT_STYLE_DEFAULT);
                paint_draw_point(x_center + s_count_y, y_center + x_current, color, DOT_PIXEL_DEFAULT, DOT_STYLE_DEFAULT);
            }
            if (esp < 0)
                esp += 4 * x_current + 6;
            else
            {
                esp += 10 + 4 * (x_current - y_current);
                y_current--;
            }
            x_current++;
        }
    }
    else
    {
        while (x_current <= y_current)
        {
            paint_draw_point(x_center + x_current, y_center + y_current, color, line_width, DOT_STYLE_DEFAULT);
            paint_draw_point(x_center - x_current, y_center + y_current, color, line_width, DOT_STYLE_DEFAULT);
            paint_draw_point(x_center - y_current, y_center + x_current, color, line_width, DOT_STYLE_DEFAULT);
            paint_draw_point(x_center - y_current, y_center - x_current, color, line_width, DOT_STYLE_DEFAULT);
            paint_draw_point(x_center - x_current, y_center - y_current, color, line_width, DOT_STYLE_DEFAULT);
            paint_draw_point(x_center + x_current, y_center - y_current, color, line_width, DOT_STYLE_DEFAULT);
            paint_draw_point(x_center + y_current, y_center - x_current, color, line_width, DOT_STYLE_DEFAULT);
            paint_draw_point(x_center + y_current, y_center + x_current, color, line_width, DOT_STYLE_DEFAULT);

            if (esp < 0)
                esp += 4 * x_current + 6;
            else
            {
                esp += 10 + 4 * (x_current - y_current);
                y_current--;
            }
            x_current++;
        }
    }
}

/**
 * @brief 绘制单个ASCII字符
 */
void paint_draw_char(u16 x_point, u16 y_point, char ch,
                     s_font_t *font, u16 color_fg, u16 color_bg)
{
    u16 page, column;

    if (x_point > paint.width || y_point > paint.height)
    {
        GUI_LOG("draw char out of range");
        return;
    }

    uint32_t char_offset = (ch - ' ') * font->height * (font->width / 8 + (font->width % 8 ? 1 : 0));
    const u8 *ptr = &font->table[char_offset];

    for (page = 0; page < font->height; page++)
    {
        for (column = 0; column < font->width; column++)
        {
            if (FONT_BACKGROUND == color_bg)
            {
                if (*ptr & (0x80 >> (column % 8)))
                    paint_set_pixel(x_point + column, y_point + page, color_fg);
            }
            else
            {
                if (*ptr & (0x80 >> (column % 8)))
                {
                    paint_set_pixel(x_point + column, y_point + page, color_fg);
                }
                else
                {
                    paint_set_pixel(x_point + column, y_point + page, color_bg);
                }
            }
            if (column % 8 == 7)
                ptr++;
        }
        if (font->width % 8 != 0)
            ptr++;
    }
}

/**
 * @brief 绘制英文字符串
 */
void paint_draw_string_en(u16 x_start, u16 y_start, const char *str,
                          s_font_t *font, u16 color_fg, u16 color_bg)
{
    u16 x_point = x_start;
    u16 y_point = y_start;

    if (x_start > paint.width || y_start > paint.height)
    {
        GUI_LOG("draw string en out of range");
        return;
    }

    while (*str != '\0')
    {
        if ((x_point + font->width) > paint.width)
        {
            x_point = x_start;
            y_point += font->height;
        }

        if ((y_point + font->height) > paint.height)
        {
            x_point = x_start;
            y_point = y_start;
        }

        paint_draw_char(x_point, y_point, *str, font, color_fg, color_bg);

        str++;
        x_point += font->width;
    }
}

/**
 * @brief 中英文混合字符串绘制（GBK中文）
 */
void paint_draw_string_cn(u16 x_start, u16 y_start, const char *str, c_font_t *font,
                          u16 color_fg, u16 color_bg)
{
    const char *p_text = str;
    int x = x_start, y = y_start;
    int i, j, num;

    while (*p_text != 0)
    {
        if (*p_text <= 0xE0)
        {
            for (num = 0; num < font->size; num++)
            {
                if (*p_text == font->table[num].index[0])
                {
                    const char *ptr = &font->table[num].matrix[0];
                    for (j = 0; j < font->height; j++)
                    {
                        for (i = 0; i < font->width; i++)
                        {
                            if (FONT_BACKGROUND == color_bg)
                            {
                                if (*ptr & (0x80 >> (i % 8)))
                                {
                                    paint_set_pixel(x + i, y + j, color_fg);
                                }
                            }
                            else
                            {
                                if (*ptr & (0x80 >> (i % 8)))
                                {
                                    paint_set_pixel(x + i, y + j, color_fg);
                                }
                                else
                                {
                                    paint_set_pixel(x + i, y + j, color_bg);
                                }
                            }
                            if (i % 8 == 7)
                            {
                                ptr++;
                            }
                        }
                        if (font->width % 8 != 0)
                        {
                            ptr++;
                        }
                    }
                    break;
                }
            }
            p_text += 1;
            x += font->ascii_width;
        }
        else
        {
            for (num = 0; num < font->size; num++)
            {
                if ((*p_text == font->table[num].index[0]) &&
                    (*(p_text + 1) == font->table[num].index[1]) &&
                    (*(p_text + 2) == font->table[num].index[2]))
                {
                    const char *ptr = &font->table[num].matrix[0];

                    for (j = 0; j < font->height; j++)
                    {
                        for (i = 0; i < font->width; i++)
                        {
                            if (FONT_BACKGROUND == color_bg)
                            {
                                if (*ptr & (0x80 >> (i % 8)))
                                {
                                    paint_set_pixel(x + i, y + j, color_fg);
                                }
                            }
                            else
                            {
                                if (*ptr & (0x80 >> (i % 8)))
                                {
                                    paint_set_pixel(x + i, y + j, color_fg);
                                }
                                else
                                {
                                    paint_set_pixel(x + i, y + j, color_bg);
                                }
                            }
                            if (i % 8 == 7)
                            {
                                ptr++;
                            }
                        }
                        if (font->width % 8 != 0)
                        {
                            ptr++;
                        }
                    }
                    break;
                }
            }
            p_text += 3;
            x += font->width;
        }
    }
}

/**
 * @brief 绘制整数
 */
void paint_draw_num(u16 x_point, u16 y_point, int32_t num,
                    s_font_t *font, u16 color_fg, u16 color_bg)
{
    int16_t num_bit = 0, str_bit = 0;
    u8 str_array[255] = {0}, num_array[255] = {0};
    u8 *p_str = str_array;

    if (x_point > paint.width || y_point > paint.height)
    {
        GUI_LOG("draw num out of range");
        return;
    }

    do
    {
        num_array[num_bit] = num % 10 + '0';
        num_bit++;
        num /= 10;
    } while (num);

    while (num_bit > 0)
    {
        str_array[str_bit] = num_array[num_bit - 1];
        str_bit++;
        num_bit--;
    }

    paint_draw_string_en(x_point, y_point, (const char *)p_str, font, color_fg, color_bg);
}

/**
 * @brief 绘制带小数浮点数
 */
void paint_draw_num_decimals(u16 x_point, u16 y_point, double num,
                             s_font_t *font, u8 digit, u16 color_fg, u16 color_bg)
{
    int16_t num_bit = 0, str_bit = 0;
    u8 str_array[255] = {0}, num_array[255] = {0};
    u8 *p_str = str_array;
    int temp = (int)num;
    float decimals;
    u8 i;

    if (x_point > paint.width || y_point > paint.height)
    {
        GUI_LOG("draw decimal out of range");
        return;
    }

    if (digit > 0)
    {
        decimals = num - temp;
        for (i = digit; i > 0; i--)
        {
            decimals *= 10;
        }
        temp = (int)decimals;
        for (i = digit; i > 0; i--)
        {
            num_array[num_bit] = temp % 10 + '0';
            num_bit++;
            temp /= 10;
        }
        num_array[num_bit] = '.';
        num_bit++;
    }

    temp = (int)num;
    do
    {
        num_array[num_bit] = temp % 10 + '0';
        num_bit++;
        temp /= 10;
    } while (temp);

    while (num_bit > 0)
    {
        str_array[str_bit] = num_array[num_bit - 1];
        str_bit++;
        num_bit--;
    }

    paint_draw_string_en(x_point, y_point, (const char *)p_str, font, color_fg, color_bg);
}

/**
 * @brief 绘制HH:MM:SS时间
 */
void paint_draw_time(u16 x_start, u16 y_start, paint_time_t *p_time, s_font_t *font,
                     u16 color_fg, u16 color_bg)
{
    u8 value[10] = {'0', '1', '2', '3', '4', '5', '6', '7', '8', '9'};
    u16 dx = font->width;

    paint_draw_char(x_start, y_start, value[p_time->hour / 10], font, color_fg, color_bg);
    paint_draw_char(x_start + dx, y_start, value[p_time->hour % 10], font, color_fg, color_bg);
    paint_draw_char(x_start + dx + dx / 4 + dx / 2, y_start, ':', font, color_fg, color_bg);
    paint_draw_char(x_start + dx * 2 + dx / 2, y_start, value[p_time->minute / 10], font, color_fg, color_bg);
    paint_draw_char(x_start + dx * 3 + dx / 2, y_start, value[p_time->minute % 10], font, color_fg, color_bg);
    paint_draw_char(x_start + dx * 4 + dx / 2 - dx / 4, y_start, ':', font, color_fg, color_bg);
    paint_draw_char(x_start + dx * 5, y_start, value[p_time->second / 10], font, color_fg, color_bg);
    paint_draw_char(x_start + dx * 6, y_start, value[p_time->second % 10], font, color_fg, color_bg);
}

/**
 * @brief 直接拷贝完整打包位图
 */
void paint_draw_bitmap(const u8 *buf)
{
    u16 x, y;
    u32 addr = 0;

    for (y = 0; y < paint.height_byte; y++)
    {
        for (x = 0; x < paint.width_byte; x++)
        {
            addr = x + y * paint.width_byte;
            paint.buf[addr] = (u8)buf[addr];
        }
    }
}

/**
 * @brief 贴图：1字节/像素图像渲染到画布，自动旋转镜像
 */
void paint_blit_image(u16 x_start, u16 y_start, const u8 *pixel_buf, u16 img_width, u16 img_height)
{
    if (!pixel_buf || !paint.buf)
        return;

    u16 x, y;
    for (y = 0; y < img_height; y++)
    {
        for (x = 0; x < img_width; x++)
        {
            u16 color = pixel_buf[y * img_width + x];
            paint_set_pixel(x_start + x, y_start + y, color);
        }
    }
}
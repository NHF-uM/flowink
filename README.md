# 使用说明

Release 目录下的`Instruction.md`有详细说明

# 开发说明

## 硬件搭建和代码烧录

### 硬件需求

ESP32-S3-N16R8（对SPRAM大小有要求，具体未测试）
元太7.3寸E6全彩墨水屏
32GB TF卡（非牌子货可能不支持SPI读取，容量可缩减）
闲鱼“记得带马扎”-50P墨水屏SPI转换器
TF 卡模块（支持SPI接口）

### 引脚连接

| 功能 | GPIO | 说明 |
| ---- | ---- | ---- |
| 电源按键 | GPIO7 | |
| 模式按键 | GPIO8 | |
| 电源指示灯 | GPIO3 | |
| 模式指示灯 | GPIO4 | |
| SCLK | GPIO10 | EPD SPI 时钟 |
| MOSI | GPIO9 | EPD SPI 主机输出 |
| CS | GPIO11 | EPD 片选 |
| DC | GPIO12 | EPD 数据 / 命令脚，高有效 |
| RST | GPIO13 | EPD 复位脚，高有效 |
| BUSY | GPIO14 | EPD 忙信号，高有效，带上拉 |
| CS | GPIO15 | SD 卡软件片选，低电平有效 |
| SCLK | GPIO16	| SD 卡 SPI 时钟，最高 24MHz|
| MISO | GPIO17 | SD 卡主机输入 |
| MOSI | GPIO18 | SD 卡主机输出 |

### 代码烧录

烧录网站：https://wiki.wireless-tag.com/tools/?tool=flash
烧录地址：0x0

## 开发环境

West version: v1.5.0

Zephyr version: 4.4.2

## 代码使用和参考

使用或参考以下开源代码：
Multi_button：[0x1abin/MultiButton: Button driver for embedded system](https://github.com/0x1abin/MultiButton)

（微雪官方）BMP解码、BMP转墨水屏数据、HTTP网页构建相关代码：

[7.3inch e-Paper HAT (E) Manual - Waveshare Wiki](https://www.waveshare.net/wiki/7.3inch_e-Paper_HAT_(E)_Manual?u_atoken=6aa1341c-ce63-b2dc-7eab-f79c569f358e&u_asig=3df10e2517889495319163210e#.E5.9B.BE.E7.89.87.E6.95.B0.E6.8D.AE.E8.BD.AC.E6.8D.A2)


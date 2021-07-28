#ifndef INCLUDE_CONSOLE_H_
#define INCLUDE_CONSOLE_H_

#include "types.h"
#include "vmm.h"

#define	rc_black  0
#define	rc_blue  1
#define	rc_green  2
#define	rc_cyan  3
#define	rc_red  4
#define	rc_magenta  5
#define	rc_brown  6
#define	rc_light_grey  7
#define	rc_dark_grey  8
#define	rc_light_blue  9
#define	rc_light_green  10
#define	rc_light_cyan  11
#define	rc_light_red  12
#define	rc_light_magenta  13
#define	rc_light_brown   14 	
#define	rc_white  15

// 清屏操作
void console_clear();

// 屏幕输出一个字符(带颜色)
void console_putc_color(char c, uint8_t back, uint8_t fore);

// 屏幕打印一个以 \0 结尾的字符串(默认黑底白字)
void console_write(char *cstr);

// 屏幕打印一个以 \0 结尾的字符串(带颜色)
void console_write_color(char *cstr, uint8_t back, uint8_t fore);

// 屏幕输出一个十六进制的整型数
void console_write_hex(uint32_t n, uint8_t back, uint8_t fore);

// 屏幕输出一个十进制的整型数
void console_write_dec(uint32_t n, uint8_t back, uint8_t fore);

#endif  // INCLUDE_CONSOLE_H_
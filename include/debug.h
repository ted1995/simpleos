#ifndef INCLUDE_DEBUG_H_
#define	INCLUDE_DEBUG_H_

#include "console.h"
#include "vargs.h"
#include "types.h"
#include "elf.h"


#define assert(x,info) do{if(!(x)){panic(info);}}while(0)

//编译期间静态监测
#define static_assert(x) switch(x){case 0:case(x):;}

//初始化debug信息
void init_debug();

//打印当前的函数调用栈信息
void panic(const char* msg);

//打印当前的段寄存器值
void printf_cur_status();

//内核的打印函数
void printk(const char* format, ...);

//内核的带颜色打印函数
void printk_color(uint8_t back,uint8_t fore,const char* format, ...);

#endif
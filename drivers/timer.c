#include "timer.h"
#include "debug.h"
#include "common.h"
#include "idt.h"
#include "console.h"
#include "sched.h"


void timer_callback(pt_regs* regs)
{
	//tick相当于系统的心跳，滴答，不断向上累加
	static uint32_t tick=0;
	tick++;
	schedule();
}



void wait(int n)
{
	int m = 10000;

	while(n)
	{
		while(m)
		{
			m--;
		}
		m = 10000;
		n--;
	}
	return 0;
}


//初始化定时器，设置定时器中断处理函数
void init_timer(uint32_t frequency)
{
	//为32号中断注册中断处理函数，只要中断到来，就
	//IRQ0=32
	register_interrupt_handler(IRQ0,timer_callback);

	// Intel 8253/8254 芯片PIT I/端口地址范围是O40h~43h
	// 输入频率为， 1193180frequency 即每秒中断次数
	uint32_t divisor = 1193180 / frequency;

	// D7 D6 D5 D4 D3 D2 D1 D0
	// 0  0  1  1  0  1  1  0
	// 即就是 36 H
	// 设置 8253/8254 芯片工作在模式 3 下
	outb(0x43, 0x36);

	// 拆分低字节和高字节
	uint8_t low = (uint8_t)(divisor & 0xFF);
	uint8_t hign = (uint8_t)((divisor >> 8) & 0xFF);
	
	// 分别写入低字节和高字节
	outb(0x40, low);
	outb(0x40, hign);
}
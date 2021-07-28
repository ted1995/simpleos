#include "console.h"
#include "common.h"

/*
 * VGA(Video Graphics Array，视频图形阵列)是使用模拟信号的一种视频传输标准，内核可以通过它来控制屏幕上字符或者图形的显示。
 * 在默认的文本模式(Text-Mode)下，VGA控制器保留了一块内存(0x8b000~0x8bfa0)=4000个字节，两个字节代表一个字符，一共表示2000个字符
 作为屏幕上字符显示的缓冲区，
 * 若要改变屏幕上字符的显示，只需要修改这块内存就好了。
 *
 */

// VGA 的显示缓冲的起点是 0xB8000，是一个地址，将其转化为16位的指针，对这个指针的操作直接操作两个字节。
static uint16_t *video_memory = (uint16_t *)(0xB8000+PAGE_OFFSET);

// 屏幕光标的坐标""
static uint8_t cursor_x = 0;
static uint8_t cursor_y = 0;


// 移动光标
static void move_cursor()
{
	// 屏幕是 80 字节宽
	uint16_t cursorLocation = cursor_y * 80 + cursor_x;
	
	// VGA 内部的寄存器多达300多个，显然无法一一映射到I/O端口的地址空间。
	// 对此 VGA 控制器的解决方案是，将一个端口作为内部寄存器的索引：0x3D4，
	// 再通过 0x3D5 端口来设置相应寄存器的值。
	// 在这里用到的两个内部寄存器的编号为14与15，分别表示光标位置的高8位与低8位。

	//是向端口3D4写入14,表示要向寄存器14写入数据
	outb(0x3D4, 14);                  	// 告诉 VGA 我们要设置光标的高字节
	//通过端口15向寄存器14写入数据
	outb(0x3D5, cursorLocation >> 8); 	// 发送高 8 位
	outb(0x3D4, 15);                  	// 告诉 VGA 我们要设置光标的低字节
	outb(0x3D5, cursorLocation);     	// 发送低 8 位
}


// 清屏操作
void console_clear()
{
	//0代表白色，高四位，15代表黑色，低四位，相或，组成一个8位的一个字节，意义为白底黑字
	uint8_t attribute_byte = (0 << 4) | (15 & 0x0F);
	//从缓存起点每两个字节代表一个字符，前一个字节代表字符，后一个字节代表格式，空格的ascii码是32=0x20,低8位前一个字节，字符属性高八位后一个字节
	uint16_t blank = 0x20 | (attribute_byte << 8);

	int i;
	//依次写入缓冲区中
	for (i = 0; i < 80 * 25; i++) 
	{
	      video_memory[i] = blank;
	}
	//重设光标的位置到起点
	cursor_x = 0;
	cursor_y = 0;
	move_cursor();
}



// 屏幕滚动操作
static void scroll()
{
	// attribute_byte 被构造出一个黑底白字的描述格式
	uint8_t attribute_byte = (0 << 4) | (15 & 0x0F);
	uint16_t blank = 0x20 | (attribute_byte << 8);  // space 是 0x20

	// cursor_y 到 25 的时候，就该换行了
	if (cursor_y >= 25) {
		// 将所有行的显示数据复制到上一行，第一行永远消失了...
		int i;
		for (i = 0 * 80; i < 24 * 80; i++) 
		{
			  //把下一行数据移到上一行
		      video_memory[i] = video_memory[i+80];
		}

		// 最后的一行数据现在填充空格，不显示任何字符
		for (i = 24 * 80; i < 25 * 80; i++) {
		      video_memory[i] = blank;
		}

		// 向上移动了一行，所以 cursor_y 现在是 24
		cursor_y = 24;
	}
}



// 屏幕输出一个字符(带颜色)
void console_putc_color(char c, uint8_t back, uint8_t fore)
{
	uint8_t back_color = back;
	uint8_t fore_color = fore;

	uint8_t attribute_byte = (back_color << 4) | (fore_color & 0x0F);
	uint16_t attribute = attribute_byte << 8;

	// 0x08 是 退格键 的 ASCII 码
	// 0x09 是 tab 键 的 ASCII 码
	if (c == 0x08 && cursor_x) 
	{
	      cursor_x--;
	} 
	else if (c == 0x09) 
	{
	      cursor_x = (cursor_x+8) & ~(8-1);
	 // /r使光标回到行首
	} 
	else if (c == '\r') 
	{
	      cursor_x = 0;
	// /n使光标去到下一行
	} 
	else if (c == '\n') 
	{
		cursor_x = 0;
		cursor_y++;
	} 
	else if (c >= ' ') 
	{
		video_memory[cursor_y*80 + cursor_x] = c | attribute;
		cursor_x++;
	}

	// 每 80 个字符一行，满80就必须换行了
	if (cursor_x >= 80) {
		cursor_x = 0;
		cursor_y ++;
	}

	// 如果需要的话滚动屏幕显示
	scroll();

	// 移动硬件的输入光标
	move_cursor();
}



// 屏幕打印一个以 \0 结尾的字符串(默认黑底白字)
void console_write(char *cstr)
{
	while (*cstr) 
	{
	    console_putc_color(*cstr++, rc_black, rc_white);
	}
}

// 屏幕打印一个以 \0 结尾的字符串(带颜色)
void console_write_color(char *cstr, uint8_t back, uint8_t fore)
{
	while (*cstr) 
	{
	    console_putc_color(*cstr++, back, fore);
	}
}


// 屏幕输出一个十六进制的整型数
void console_write_hex(uint32_t n, uint8_t back, uint8_t fore)
{
	int tmp;
	char noZeroes = 1;

	console_write_color("0x", back, fore);

	int i;
	for (i = 28; i >= 0; i -= 4) 
	{
		//依次取出32位字符的高4位，中高4位，中低4位，低4位。。。，一共8个字节
		tmp = (n >> i) & 0xF;
		//为0直接忽略
		if (tmp == 0 && noZeroes != 0) 
		{
		      continue;
		}
		noZeroes = 0;
		//对于十六进制 的A—F
		if (tmp >= 0xA) 
		{
			//以’a‘为基准加上偏移，输出相应的字符
		      console_putc_color(tmp-0xA+'a', back, fore);
		} 
		else 
		{	//小于A的，直接转化为字符输出
		      console_putc_color(tmp+'0', back, fore);
		}
	}
}


// 屏幕输出一个十进制的整型数
void console_write_dec(uint32_t n, uint8_t back, uint8_t fore)
{
	if (n == 0) {
		console_putc_color('0', back, fore);
		return;
	}

	uint32_t acc = n;
	char c[32];
	int i = 0;
	while (acc > 0) {
		c[i] = '0' + acc % 10;
		acc /= 10;
		i++;
	}
	c[i] = 0;

	char c2[32];
	c2[i--] = 0;

	int j = 0;
	while(i >= 0) {
	      c2[i--] = c[j++];
	}

	console_write_color(c2, back, fore);
}
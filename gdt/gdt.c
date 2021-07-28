#include "gdt.h"
#include "string.h"


//全局描述附表长度
#define GDT_LENGTH 5

//全局描述符表定义即全局描述符数组
gdt_entry_t gdt_entries[GDT_LENGTH];

// GDTR
gdt_ptr_t gdt_ptr;

// 全局描述符表构造函数，根据下标构造
static void gdt_set_gate(int32_t num, uint32_t base, uint32_t limit, uint8_t access, uint8_t gran);

// 声明内核栈地址
extern uint32_t stack;


// 初始化全局描述符表
void init_gdt()
{
	//初始化GDTR寄存器的值，包括表界限和表基址
	// 全局描述符表界限,由全局描述符个数×单个描述符占用的字节数得到描述符表的总字节长度，下标从0开始，因此最后再减1
	gdt_ptr.limit = sizeof(gdt_entry_t) * GDT_LENGTH - 1;
	// 全局描述符表基址初始化为全局描述符数组起始地址
	gdt_ptr.base = (uint32_t)&gdt_entries;

	// 采用 Intel 平坦模型
	gdt_set_gate(0, 0, 0, 0, 0);             	// 按照 Intel 文档要求，第一个描述符必须全 0
	gdt_set_gate(1, 0, 0xFFFFFFFF, 0x9A, 0xCF); 	// 内核代码段
	gdt_set_gate(2, 0, 0xFFFFFFFF, 0x92, 0xCF); 	// 内核数据段
	gdt_set_gate(3, 0, 0xFFFFFFFF, 0xFA, 0xCF); 	// 用户代码段
	gdt_set_gate(4, 0, 0xFFFFFFFF, 0xF2, 0xCF); 	// 用户数据段


	// 加载全局描述符表地址到 GDTR 寄存器
	gdt_flush((uint32_t)&gdt_ptr);
}


// 全局描述符表构造函数，根据下标构造
// 参数分别是 数组下标、基地址、限长、访问标志，其它访问标志
static void gdt_set_gate(int32_t num, uint32_t base, uint32_t limit, uint8_t access, uint8_t gran)
{
	//设置基址
	gdt_entries[num].base_low     = (base & 0xFFFF);
	gdt_entries[num].base_middle  = (base >> 16) & 0xFF;
	gdt_entries[num].base_high    = (base >> 24) & 0xFF;
	//
	gdt_entries[num].limit_low    = (limit & 0xFFFF);
	gdt_entries[num].granularity  = (limit >> 16) & 0x0F;

	gdt_entries[num].granularity |= gran & 0xF0;
	gdt_entries[num].access       = access;
}
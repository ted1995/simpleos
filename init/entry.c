#include "common.h"
#include "console.h"
#include "string.h"
#include "debug.h"
#include "gdt.h"
#include "idt.h"
#include "timer.h"
#include "pmm.h"
#include "vmm.h"
#include "heap.h"
#include "sched.h"

// 内核初始化函数
void kern_init();

// 开启分页机制之后的 Multiboot 数据指针
multiboot_t *glb_mboot_ptr;

// 开启分页机制之后的内核栈
char kern_stack[STACK_SIZE];



// 内核栈的栈顶
uint32_t kern_stack_top;

extern uint32_t t_used_addr;
extern struct task_struct *idle;

//临时代码段和数据段放在从0x10000处开始计算偏移地址，类似于以前的内核，grub加载内核时正好0x10000处加载
//因此，grub会从这里开始运行临时段开始加载



//代表临时页表和页目录放在临时数据段
__attribute__((section(".init.data"))) pgd_t *pgd_tmp = (pgd_t *)0x1000;
__attribute__((section(".init.data"))) pgd_t *pte_low = (pgd_t *)0x2000;
__attribute__((section(".init.data"))) pgd_t *pte_hign = (pgd_t *)0x3000;


//代表kern——entry放在临时代码段
__attribute__((section(".init.text"))) void kern_entry()
{

	//pgd_tmp是页目录，pte_low与pte_hign是页表地址
	pgd_tmp[0] = (uint32_t)pte_low | PAGE_PRESENT | PAGE_WRITE;
	
	pgd_tmp[PGD_INDEX(PAGE_OFFSET)] = (uint32_t)pte_hign | PAGE_PRESENT |PAGE_WRITE;

	//映射低端4M是因为，kern——entry函数是在低端地址运行的，在此函数内部开始分页机制后，如果没有映射低端地址，开启分页之后的函数语句将会找不到映射的物理地址而无法执行
	//映射 0x00000000−0x00400000 的物理地址到虚拟地址 0x00000000−0x00400000
	//8K地址处存放的是0x0对应的页表，页表映射的是0～4M的物理内存的1024个内存页
	int i;
	for (i = 0; i < 1024; i++) 
	{
		pte_low[i] = (i << 12) | PAGE_PRESENT | PAGE_WRITE;
	}

	//映射高端3G地址是为了后面的kern——init函数执行的时候，此函数语句已经从0xc00000000开始偏移，能够找到正确的映射的物理地址去执行
	// 映射 0x00000000−0x00400000 的物理地址到虚拟地址 0xC0000000−0xC0400000
	//12k地址处存放的是0xC0000000对应的页表，页表映射的是0～4M的物理内存的1024个内存页
	for (i = 0; i < 1024; i++)
	{
		pte_hign[i] = (i << 12) | PAGE_PRESENT | PAGE_WRITE;
	}


	//将页目录物理基址放入cr3寄存器
	asm volatile ("mov %0, %%cr3" : : "r" (pgd_tmp));


	uint32_t cr0;

	// 启用分页，将 cr0 寄存器的分页位置为 1 就好
	asm volatile ("mov %%cr0, %0" : "=r" (cr0));
	cr0 |= 0x80000000;
	asm volatile ("mov %0, %%cr0" : : "r" (cr0));

	// 切换内核栈
	kern_stack_top = ((uint32_t)kern_stack + STACK_SIZE) & 0xFFFFFFF0;
	asm volatile ("mov %0, %%esp\n\t"
			"xor %%ebp, %%ebp" : : "r" (kern_stack_top));

	// 更新全局 multiboot_t 指针
	glb_mboot_ptr = mboot_ptr_tmp + PAGE_OFFSET;

	// 调用内核初始化函数
	kern_init();

}


int flag = 0;

int thread5(void *arg)
{
	int i=100;
	while (i) 
	{
		wait(50);
		printk_color(rc_black, rc_green, "i am thread5\n");
		i--;
	}
	return 0;
}


int thread5_1(void *arg)
{
	while (1) 
	{
		wait(50);
		printk_color(rc_black, rc_red, "i am thread5_1\n");
	}
	return 0;
}


int thread6(void *arg)
{
	while (1) 
	{
		wait(500);
		printk_color(rc_black, rc_blue, "i am thread6\n");
	}
	return 0;
}


void idle_task()
{
	while(1)
	{
		wait(50);
		printk("i am idle!\n");
	}
}


void kern_init()
{

	
	//清屏
	console_clear();

	printk("kern_start=%x\n",kern_start);
	printk("kern_end=%x\n",kern_end);

	//初始化调试，接收multiboot结构体中的数据供我们自己使用
	init_debug();
	//初始化全局描述符表，cpu进入保护模式运行
	init_gdt();
	//初始化中断描述符表，设置相关的中断处理函数
	init_idt();

	// 注册页错误中断的处理函数 ( 14 是页故障的中断号 )
	register_interrupt_handler(14, &page_fault);
	
	//初始化定时器，系统的心跳，在系统的心跳处理函数中运行调度任务
	init_timer(200);

	printk_color(rc_black,rc_green,"hello,welcome to myos!\n");

	//初始化物理内存页,将1M以上且内核存放位置以外的物理内存划分为一个个4K大小的页
	//init_pmm();

	//物理内存管理-伙伴算法
	init_pmm1();

	pm_alloc_t my_alloc=pmm_alloc_pages(1);
	printk("addr=%d\n",addr_to_pmm_page_no(my_alloc.addr));
	printk("size=%d\n",my_alloc.size);

	pmm_free_pages(my_alloc);

	
	//创建一个二级页表，将512M的内存全部映射到虚拟地址0xc00000000以上的位置，
	//init_vmm();

	//将虚拟地址0xe000000以上的地址作为堆地址，可以进行动态内存的申请malloc与释放free
	//init_heap();

	//test_heap();

	//将当前执行流添加为一个内核线程，同时新建一个idle线程，在内核第一次调度时从当前执行流切换到idle线程
	//init_sched(idle_task);

	//基于优先级的位图调度算法
	//创建一个新线程任务
	//kernel_thread(thread5,NULL,5,2);
	//kernel_thread(thread5_1,NULL,5,2);
	//pkernel_thread(thread6,NULL,6,2);

	// 开启中断,定时器中断周期调度
	//asm volatile ("sti");

	while (1) 
	{
		asm volatile ("hlt");
	}
}


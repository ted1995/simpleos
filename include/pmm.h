#ifndef INCLUDE_PMM_H_
#define INCLUDE_PMM_H_

#include "multiboot.h"

//内核文件在内存中的起始和结束位置
extern	uint8_t	kern_start[];
extern	uint8_t kern_end[];


//伙伴管理的物理内存页数，64M内存
#define buddy_page_number 8192

// 线程栈的大小
#define STACK_SIZE 8192

// 支持的最大物理内存(512MB)
#define PMM_MAX_SIZE 0x20000000

// 物理内存页框大小
#define PMM_PAGE_SIZE 0x1000

// 最多支持的物理页面个数
#define PAGE_MAX_SIZE (PMM_MAX_SIZE/PMM_PAGE_SIZE)

// 页掩码按照 4096 对齐地址
#define PHY_PAGE_MASK 0xFFFFF000

// 动态分配物理内存页的总数
extern uint32_t phy_page_count;


#define ERRO_POP_BLOCK 0xFFFFFFFF    //无法寻找页的返回

//链表节点
typedef 
struct pm_page_t{
	uint32_t page_no;			//当前内存块对应的起始物理页编号
	struct pm_page_t * next;	//下一个节点
	uint8_t state;	// 1--空闲可用  0--不可用 
} pm_page_t;


//伙伴算法的11条链表结构体
typedef
struct pm_multi_link_t{
	pm_page_t * list_1;
	pm_page_t * list_2;
	pm_page_t * list_4;
	pm_page_t * list_8;
	pm_page_t * list_16;
	pm_page_t * list_32;
	pm_page_t * list_64;
	pm_page_t * list_128;
	pm_page_t * list_256;
	pm_page_t * list_512;
	pm_page_t * list_1024;
} pm_multi_link_t;



//定义块大小的枚举
typedef 
enum page_c_t{
	list_1=0,
	list_2=1,
	list_4=2,
	list_8=3,
	list_16=4,
	list_32=5,
	list_64=6,
	list_128=7,
	list_256=8,
	list_512=9,
	list_1024=10,
	list_erro=11
} page_c_t;


//分配页时返回的结构体 注意free页时也必须用此结构体 这样可以帮助MUTI链表找到free块后插入哪个子链表
typedef 
struct pm_alloc_t{
	uint32_t addr;     //分配页的地址
	page_c_t size;     //分配的页数量
	uint8_t state;     //为1 分配成功 为0 分配失败  分配后必须先判断此字段
} pm_alloc_t;





void init_pmm1();

void append_block(int page_no,page_c_t c);

uint32_t pop_block(page_c_t c);

uint32_t alloc_helper(page_c_t target_ph,page_c_t pop_ph,uint32_t pop_page_no);

uint32_t pmm_page_no_to_addr(uint32_t page_no);

uint32_t addr_to_pmm_page_no(uint32_t addr);

uint32_t c_to_uint32(page_c_t ph);

uint32_t get_partner_page_no(uint32_t page_no,page_c_t type);

pm_alloc_t pmm_alloc_pages(uint32_t page_count);

void pmm_free_pages(pm_alloc_t block_disc);

void show_memory_map();

void init_pmm();

uint32_t pmm_alloc_page();

uint32_t pmm_buddy_alloc_page();

void pmm_free_page(uint32_t p);

void pmm_buddy_free_page(uint32_t p);


#endif
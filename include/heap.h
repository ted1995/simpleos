#ifndef INCLUDE_HEAP_H_
#define INCLUDE_HEAP_H_

#include "types.h"
#include "idt.h"
#include "string.h"

//内核堆管理，用于动态的申请内存域释放内存

// 堆起始地址，有最大500多M的连续空间
#define HEAP_START 0xE0000000

#define TMP_HEAP_START 0xD0000000

#define INDEX_SIZE 11


#define USED 1
#define UNUSED 0

// 内存块管理结构
typedef
struct header {
	struct header *prev; 	// 前后内存块管理结构指针
	struct header *next;
	uint32_t allocated : 1;	// 该内存块是否已经被申请
	uint32_t length : 31; 	// 当前内存块的长度
} header_t;


//对堆500多M的连续内存运用伙伴系统进行管理

//内存块链表结点结构 
typedef struct buddy_node {
	uint8_t flag;                       //标记空间是否被使用
	uint32_t base;                     //本块儿内存的基地址 
	uint16_t index;						//此节点对应的链表索引
	struct buddy_node *next;         //指向下一个结点 
	struct buddy_node *prev;         //指向上一个结点 
} Buddy, *BuddyPtr;


//哈希索引表结构 
typedef struct hash_table {
	uint16_t nodesize;			//此链表对应的块大小，即4k页的个数
	BuddyPtr headPtr;		//此链表对应的块头指针
	BuddyPtr tailPtr;		//此链表对应的块尾指针
	uint32_t number;		//此链表对应的空闲内存块个数
} IndexTable;


IndexTable table[INDEX_SIZE];//使用哈希表管理伙伴系统



// 初始化堆
void init_heap();

// 内存申请
void *kmalloc(uint32_t len);

// 内存释放
void kfree(void *p);

// 测试内核堆申请释放
void test_heap();

void init_index (void);

void* tmalloc(uint32_t len);

//将堆起始地址开始的内存初始化为伙伴系统
void init_buddy ();

void* bmalloc(uint32_t len);

uint8_t getindex(uint32_t tmp);

void bfree(uint32_t* p);

uint32_t push_front(IndexTable* p,uint32_t base,uint8_t i);

#endif 	// INCLUDE_HEAP_H_


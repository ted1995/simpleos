#include "debug.h"
#include "pmm.h"
#include "vmm.h"
#include "heap.h"


//对于只在堆内部调用的函数，用static声明

// 申请内存块
static void alloc_chunk(uint32_t start, uint32_t len);

// 释放内存块
static void free_chunk(header_t *chunk);

// 切分内存块
static void split_chunk(header_t *chunk, uint32_t len);

// 合并内存块
static void glue_chunk(header_t *chunk);

//堆最大值初始与堆起始一致，在申请内存的过程中不断扩大heap_max的值
static uint32_t heap_max = TMP_HEAP_START;

// 内存块管理头指针
static header_t *heap_first;

//内核结束后的起始地址
uint32_t t_addr;

//目前已经分配的地址
uint32_t t_alloc_addr;

//目前已经使用到的地址
uint32_t t_used_addr;

extern uint32_t pmm_stack_top;

uint32_t base_to_buddy[100];

//函数：根据k的值计算哈希表项链接的内存块的大小
int get_size (int k)
{
    int i, nodesize = 1;
    
    for (i = 0; i < k; i ++)
        nodesize *= 2;
 
    return nodesize;
}


//合并内存块


//函数：初始化哈希索引表 
void init_index (void)
{
    int i;
    
    for (i = 0; i < INDEX_SIZE; i ++) 
    {
        table[i].nodesize = get_size(i);
        table[i].headPtr = NULL;
        table[i].tailPtr = NULL;
        table[i].number=0;
    } 
}



//函数：//将堆起始地址开始的内存初始化为伙伴系统
void init_buddy ()
{
	//先获取内核后的起始地址
	uint32_t tmp_end=kern_end;
	//4k对齐
	t_addr=(uint32_t*)((tmp_end+PMM_PAGE_SIZE) & PHY_PAGE_MASK);

	//初始化已经使用和分配的地址
	t_alloc_addr=t_addr;
	t_used_addr=t_addr;
	//以上参数用来临时的tmalloc来分配内存块管理结构体占用的内存
	

	//伙伴系统内存块虚拟地址起始地址
	uint32_t tmp_heap_addr=HEAP_START;

	//将内存分割成尽量大的2倍数的空闲块并插入到相应的索引表项中
	for (int i = INDEX_SIZE-1;  i>=0; i--) 
	{

		//物理页数量足够当前table[i]使用，分配物理页，构成内存块,循环直到当前物理页不够当前table[i]，则换到下一条table[i]
 		while(pmm_stack_top>=table[i].nodesize)
 		{
 			printk("tmp_heap_addr=%x ",tmp_heap_addr);
 			push_front(&table[i],tmp_heap_addr,i);
 			//为内存块分配(table[i].nodesize个物理页
		    for(uint16_t k=0;k<table[i].nodesize;k++)
	    	{
	    		uint32_t page=pmm_alloc_page();
	    		map(pgd_kern,tmp_heap_addr,page,PAGE_PRESENT | PAGE_WRITE);
	    		tmp_heap_addr+=PAGE_SIZE;
	    	}
		}
 	}
}



void div0()
{
	while(1)
	{

	}
}


//初始化伙伴系统，将一块连续的的虚拟内存分成不同大小的块，存在各自的链表中
void init_heap()
{
	register_interrupt_handler(0,&div0);
	heap_first=0;
	init_index();
	init_buddy();
	/*
	for(int i=INDEX_SIZE-1;i>=0;i--)
	{
		//table[i]是索引数组，内部存放内存块的管理结构体指针链表，链表头是headPtr
		//由headPtr可以找到其他内存块的管理结构体指针
		//BuddyPtr tmp=table[i].headPtr;
		printk("table[%d].number=%x\n",i,table[i].number);
		
		for(uint32_t j=0;j<table[i].number;j++)
		{
			//打印内存块的基地址
			printk("%x ",tmp->base);
			tmp=tmp->next;
		}
		printk("\n");
		
	}
*/
	
}


uint8_t getindex(uint32_t tmp)
{
	for(uint8_t i=0;i<100;i++)
	{
		if(base_to_buddy[i]==tmp)
		{
			return i;
		}
	}
	return 101;
}


//删除链表末尾的元素，并返回被删内存的结构体指针
BuddyPtr pop_back(IndexTable* p)
{
 	//取出末尾内存块
	BuddyPtr tmp_buddy=p->tailPtr;
	//如果只有一个内存块,取出后就没有了
	if(p->headPtr==p->tailPtr)
	{
		p->headPtr=NULL;
		p->tailPtr=NULL;
	}
	//不只一个内存块
	else
	{
		//尾指针移动
		p->tailPtr=p->tailPtr->prev;
	}
	//空闲块数量减1
	p->number--;
	return tmp_buddy;
}


uint32_t push_front(IndexTable* p,uint32_t base,uint8_t i)
{
	//创建一个新内存节点,用临时的tmalloc分配一个内存管理结构体
	BuddyPtr newnodePtr = (BuddyPtr)kmalloc(sizeof(Buddy));
	//printk("newnodePtr=%x ",newnodePtr);
	newnodePtr->base=base;
	newnodePtr->index=i;
	newnodePtr->flag=UNUSED;
	uint8_t buddy_index=((uint32_t)newnodePtr-0xd000000c)/32;
	//将新分配的buddy结构体与内存块地址相互映射
	base_to_buddy[buddy_index]=base;
	//如果低一级的头指针为空，则直接作为头指针
	if(p->headPtr==NULL)
	{
		p->headPtr=newnodePtr;
		p->tailPtr=newnodePtr;
		newnodePtr->prev=NULL;
		newnodePtr->next=NULL;
	}
	//头指针不为空
	else
	{
		newnodePtr->prev=NULL;
		newnodePtr->next=p->headPtr;
		p->headPtr->prev=newnodePtr;
		p->headPtr=newnodePtr;
	}
	p->number++;
	base+=p->nodesize*4096;
	return base;
}


//分配内存
void* bmalloc(uint32_t len)
{
	uint32_t ans;
	//从内存链表中找到最小能够满足此长度的内存块
	//先计算需要几个4k页
	uint16_t page_num=(len/4096)+1;
	for(int i=0;i<INDEX_SIZE;i++)
	{
		//此链表内存块页数大于等于page_num,且存在空闲块
		if(table[i].nodesize>=page_num&&table[i].number!=0)
		{
			BuddyPtr tmp_buddy=pop_back(&table[i]);

			uint8_t flag=1;
			uint32_t tmp_base=tmp_buddy->base;
			uint8_t j=i;
			//此内存块的一半仍然足够分配
			while(table[j].nodesize/2>=page_num)
			{
				flag=0;
				//如果对原来的内存块进行分割，则原来的内存块与buddy的映射就要失效了,且要释放原来的buddy结构体内存
				uint8_t index=getindex(tmp_base);
				base_to_buddy[index]=0;
				kfree(tmp_buddy);
				//循环将此内存分割一半存入低一级的内存块链表，另一半再次判断够不够分配
				tmp_base=push_front(&table[j-1],tmp_base,j-i);
				j--;
			}
			//flag为0，说明进行了内存分块，需要将最后剩余的内存块也加入链表
			if(flag==0)
			{
				//将剩余内存块加入链表头部
				push_front(&table[j],tmp_base,j);
				//将当前table[j]尾部内存块分配出去
				BuddyPtr t_buddy=pop_back(&table[j]);
				tmp_buddy->flag=USED;
				uint32_t tmp=t_buddy->base;
				table[j].number--;
				ans=tmp;
				break;
			}
			//否则说明未进行内存分块，直接将上面取出的内存块分配出去
			else
			{
				tmp_buddy->flag=USED;
				ans=tmp_base;
				break;
			}
		}
	}
	uint32_t* p=NULL;
	get_mapping(pgd_kern, ans, p);
	printk("p=%x\n",p);
	printk("ans=%x\n",ans);
	return (uint32_t*)ans;
}


void delete(IndexTable* ip,Buddy* bp)
{
	if(bp==ip->headPtr&&bp==ip->tailPtr)
	{
		ip->headPtr=NULL;
		ip->tailPtr=NULL;
	}
	else if(bp==ip->tailPtr&&bp!=ip->headPtr)
	{
		ip->tailPtr=bp->prev;
		ip->tailPtr->next=NULL;
	}
	else if(bp!=ip->tailPtr&&bp==ip->headPtr)
	{
		ip->headPtr=bp->next;
		ip->headPtr->prev=NULL;
	}
	else
	{
		bp->next->prev=bp->prev;
		bp->prev->next=bp->next;
	}
}

void bfree(uint32_t* p)
{

	//传入的p是内存块起始地址
	uint8_t index=getindex((uint32_t)p);
	//获得buddy结构体地址
	BuddyPtr tmp_buddy=(BuddyPtr)(index*32+0xd000000c);
	printk("tmp_buddy=%x\n",tmp_buddy);
	//根据此结构体判断内存快大小，由此得出前后内存块是否与此块大小相同，相同则合并
	uint8_t cur_i=tmp_buddy->index;
	printk("table[cur_i]=%d\n",table[cur_i].number);
	//已经是最顶层，不用合并
	if(cur_i==10)
	{
		kfree(tmp_buddy);
		push_front(&table[cur_i],(uint32_t)p,cur_i);
		return;
	}
	uint32_t fore_base=(uint32_t)p-table[cur_i].nodesize*4096;
	printk("fore_base=%x\n",fore_base);
	index=getindex((uint32_t)fore_base);
	if(index==101)
	{
		//没有内存块，不用合并，直接将p加入对应的链表即可
		printk("no memory block %x\n",fore_base);
		kfree(tmp_buddy);
		push_front(&table[cur_i],(uint32_t)p,cur_i);
		printk("table[cur_i]=%d\n",table[cur_i].number);
		return;
	}
	BuddyPtr tmp_buddy1=(BuddyPtr)(index*32+0xd000000c);
	//属于同一链表，内存块存在且未使用，则可以合并
	if(cur_i==tmp_buddy1->index&&tmp_buddy1->flag==UNUSED)
	{
		
		//先将前面内存块和后面内存分别从链表中拿出来
		delete(&table[cur_i],tmp_buddy);
		delete(&table[cur_i],tmp_buddy1);
		//再释放前面与后面内存块的buddy结构体
		kfree(tmp_buddy);
		kfree(tmp_buddy1);
		//再将前面内存块更改到高一级的链表中
		push_front(&table[cur_i+1],fore_base,cur_i+1);
		return;
	}
	else
	{
		printk("......\n");
		kfree(tmp_buddy);
		push_front(&table[cur_i],(uint32_t)p,cur_i);
		printk("table[cur_i]=%d\n",table[cur_i].number);
		return;
	}

}

//从内核结束后的地址分配一块len长度的内存
void* tmalloc(uint32_t len)
{
	//当前有已分配页,且剩下的长度足够分配
	if(t_alloc_addr>t_addr&&t_used_addr+len<=t_alloc_addr)
	{
		uint32_t tmp_used_addr=t_used_addr;
		//已使用的指针向后移动len
		t_used_addr+=len;
		return (void*)(uint32_t)tmp_used_addr;
	}
	//无分配页
	else if(t_alloc_addr==t_addr&&t_used_addr==t_alloc_addr)
	{
		//按照长度计算需要分配页
		uint8_t page_num=(len/4096)+1;
		for(uint8_t i=0;i<page_num;i++)
		{
			//从内存页数组中取出一个4k页的首地址返回
			uint32_t page = pmm_buddy_alloc_page();
			//将虚拟与物理映射
			map(pgd_kern,t_alloc_addr,page,PAGE_PRESENT | PAGE_WRITE);
			//虚拟向后移动一页地址，继续映射物理内存页
			t_alloc_addr+=PAGE_SIZE;
		}
		t_used_addr+=len;
		return (void*)(uint32_t)t_addr;
	}	
	//有分配页，但是不够长度
	else
	{
		uint32_t tmp_used_addr=t_used_addr;
		//按照长度计算需要分配页
		uint8_t page_num=((len-(t_alloc_addr-t_used_addr))/4096)+1;
		for(uint8_t i=0;i<page_num;i++)
		{
			//从内存页数组中取出一个4k页的首地址返回
			uint32_t page = pmm_buddy_alloc_page();
			//将虚拟与物理映射
			map(pgd_kern,t_alloc_addr,page,PAGE_PRESENT | PAGE_WRITE);
			//虚拟向后移动一页地址，继续映射物理内存页
			t_alloc_addr+=PAGE_SIZE;
		}
		t_used_addr+=len;
		return (void*)(uint32_t)tmp_used_addr;
	}
}



void *kmalloc(uint32_t len)
{
	//申请一块内存，还要加上内存快管理头的长度
	len+=sizeof(header_t);
	//printk("len=%d\n",len);

	//每次申请内存，都从头块开始遍历，找到符合要求的块，如果都不符合要求，就申请新的内存块，直到满足要求
	header_t* cur_header=heap_first;
	header_t* prev_header=0;
	//当前内存块不为空
	while(cur_header)
	{
		//当前内存块没有被申请过且长度大于待审请的块
		if (cur_header->allocated == 0 && cur_header->length >= len) 
		{
			// 按照当前长度切割内存
			split_chunk(cur_header, len);
			cur_header->allocated = 1;
			// 返回的时候必须将指针挪到管理结构之后
			return (void *)((uint32_t)cur_header + sizeof(header_t));
		}
		prev_header=cur_header;
		cur_header=cur_header->next;
	}

	//没有找到合适的内存块可以分配
	uint32_t chunk_start;

	// 之后根据当前指针加上申请的长度即可
	//如果prev_header不为空，是一个存在的内存块，则将chunk_start移到prev_header内存块之后
	if (prev_header) 
	{
		chunk_start = (uint32_t)prev_header + prev_header->length;
	} 
	// 第一次执行该函数，prev_header=0
	else 
	{
		//初始化内存块起始位置为堆起始位置
		chunk_start = TMP_HEAP_START;
		//printk("chunk_start=%x\n",chunk_start);
		//初始化内存块管理头指针，头指针不为空，以头指针结构体申请一个内存块，后面的prev就不会再为空
		heap_first = (header_t *)chunk_start;
	}

	//根据起始地址和申请的长度判断是否需要申请新的内存块，当前堆不够用1
	//printk("heap_max=%x\n",heap_max);
	if(chunk_start+len>heap_max)
	{
		//申请内存块
		alloc_chunk(chunk_start, len);
		//printk("heap_max=%x\n",heap_max);
	}
	
	//初始化新申请的内存块的管理结构体
	cur_header = (header_t *)chunk_start;
	cur_header->prev = prev_header;
	cur_header->next = 0;
	cur_header->allocated = 1;
	cur_header->length = len;
	

	//链接prev和cur
	if (prev_header) 
	{
		prev_header->next = cur_header;
	}

	//跳过管理结构体，返回真正的内存块起始地址
	return (void*)(chunk_start + sizeof(header_t));
}


void kfree(void *p)
{
	// 指针回退到管理结构，并将已使用标记置 0
	header_t *header = (header_t*)((uint32_t)p - sizeof(header_t));
	header->allocated = 0;

	// 粘合内存块
	glue_chunk(header);
}



void glue_chunk(header_t *chunk)
{
	// 如果该内存块后面有链内存块且未被使用则拼合
	if (chunk->next && chunk->next->allocated == 0) 
	{
		chunk->length = chunk->length + chunk->next->length;
		if (chunk->next->next) 
		{
			chunk->next->next->prev = chunk;
		}
		chunk->next = chunk->next->next;
	}

	// 如果该内存块前面有链内存块且未被使用则拼合
	if (chunk->prev && chunk->prev->allocated == 0) 
	{
		chunk->prev->length = chunk->prev->length + chunk->length;
		chunk->prev->next = chunk->next;
		if (chunk->next) 
		{
			chunk->next->prev = chunk->prev;
		}
		chunk = chunk->prev;
	}

	// 假如该内存后面没有链表内存块了直接释放掉
	if (chunk->next == 0) 
	{
		free_chunk(chunk);
	}
}



void free_chunk(header_t *chunk)
{
	//以chunk后面没有内存块为前提
	//chunk是头块内存
	if (chunk->prev == 0) 
	{
		heap_first = 0;
	} 
	//非头块内存
	else 
	{
		chunk->prev->next = 0;
	}

	// 空闲的内存超过 1 页的话就释放掉
	//此时chunk已经释放掉了，释放chunk申请的物理页
	while ((heap_max - PAGE_SIZE) >= (uint32_t)chunk) 
	{
		//heap_max减少4k
		heap_max -= PAGE_SIZE;
		uint32_t page;
		//获取heap_max映射的物理页地址
		get_mapping(pgd_kern, heap_max, &page);
		//取消映射
		unmap(pgd_kern, heap_max);
		//释放物理页
		pmm_buddy_free_page(page);
	}
}


void split_chunk(header_t *chunk, uint32_t len)
{
	// 切分内存块之前得保证之后的剩余内存至少容纳一个内存管理块的大小
	if (chunk->length - len > sizeof (header_t)) 
	{
		//？？？
		header_t *newchunk = (header_t *)((uint32_t)chunk + chunk->length);
		newchunk->prev = chunk;
		newchunk->next = chunk->next;
		newchunk->allocated = 0;
		newchunk->length = chunk->length - len;

		chunk->next = newchunk;
		chunk->length = len;
	}
}



void alloc_chunk(uint32_t start, uint32_t len)
{
	//循环直到申请到足够的内存块
	while (start + len > heap_max) 
	{
		//从内存页数组中取出一个4k页的首地址返回
		uint32_t page = pmm_buddy_alloc_page();
		//二级页表映射堆顶与新申请的物理页
		map(pgd_kern, heap_max, page, PAGE_PRESENT | PAGE_WRITE);
		//堆扩大了4k
		heap_max += PAGE_SIZE;
	}

}


void test_heap()
{
	printk_color(rc_black, rc_magenta, "Test kmalloc() && kfree() now ...\n\n");

	void *addr1 = kmalloc(50);
	printk("kmalloc    50 byte in 0x%X\n", addr1);
	void *addr2 = kmalloc(500);
	printk("kmalloc   500 byte in 0x%X\n", addr2);
	void *addr3 = kmalloc(5000);
	printk("kmalloc  5000 byte in 0x%X\n", addr3);
	void *addr4 = kmalloc(50000);
	printk("kmalloc 50000 byte in 0x%X\n\n", addr4);

	printk("free mem in 0x%X\n", addr1);
	kfree(addr1);
	printk("free mem in 0x%X\n", addr2);
	kfree(addr2);
	printk("free mem in 0x%X\n", addr3);
	kfree(addr3);
	printk("free mem in 0x%X\n\n", addr4);
	kfree(addr4);
}

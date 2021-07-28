
#include "common.h"
#include "debug.h"
#include "pmm.h"


//为伙伴链表结构体分配内存
pm_multi_link_t	 pm_multi_link={NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL};
//全局伙伴链表结构体指针，指向内存
pm_multi_link_t* buddy_ptr=&pm_multi_link;

//物理内存可用的起始地址
uint32_t pmm_page_start;
//物理内存可用大小
uint32_t pmm_page_size;

//物理页节点数组,初始化分配64M的物理内存页
pm_page_t page_arr[buddy_page_number];





//存储物理内存页的栈,存放的是内存页的起始地址，32位系统是32位的地址
static uint32_t pmm_stack[PAGE_MAX_SIZE+1];
static uint32_t pmm_buddy_stack[5];


//栈指针
uint32_t pmm_stack_top;
uint32_t pmm_buddy_stack_top;

//物理内存页的数量
uint32_t phy_page_count;

uint32_t memory_length;



void append_block(int page_no,page_c_t c)
{
	page_arr[page_no].next = NULL ;     //一定要设置 新加入块的下个指针为NULL
	pm_page_t * header;
	switch(c)
	{
		case list_1:
			header = buddy_ptr->list_1;
			if(header==NULL)
				buddy_ptr ->list_1 = &(page_arr[page_no]);
			break;
		case list_2:
			header = buddy_ptr ->list_2;
			if(header==NULL)
				buddy_ptr ->list_2 = &(page_arr[page_no]);
			break;
		case list_4:
			header = buddy_ptr ->list_4;
			if(header==NULL)
				buddy_ptr ->list_4 = &(page_arr[page_no]);
			break;
		case list_8:
			header = buddy_ptr ->list_8;
			if(header==NULL)
				buddy_ptr ->list_8 = &(page_arr[page_no]);
			break;
		case list_16:
			header = buddy_ptr ->list_16;
			if(header==NULL)
				buddy_ptr ->list_16 = &(page_arr[page_no]);
			break;
		case list_32:
			header = buddy_ptr ->list_32;
			if(header==NULL)
				buddy_ptr ->list_32 = &(page_arr[page_no]);
			break;
		case list_64:
			header = buddy_ptr ->list_64;
			if(header==NULL)
				buddy_ptr ->list_64 = &(page_arr[page_no]);
			break;
		case list_128:
			header = buddy_ptr ->list_128;
			if(header==NULL)
				buddy_ptr ->list_128 = &(page_arr[page_no]);
			break;
		case list_256:
			header = buddy_ptr ->list_256;
			if(header==NULL)
				buddy_ptr ->list_256 = &(page_arr[page_no]);
			break;
		case list_512:
			header = buddy_ptr ->list_512;
			if(header==NULL)
				buddy_ptr ->list_512 = &(page_arr[page_no]);
			break;
		case list_1024:
			header = buddy_ptr ->list_1024;
			if(header==NULL)
				buddy_ptr ->list_1024 = &(page_arr[page_no]);
			break;
	}
	if(header!=NULL){
		pm_page_t * probe = header;
		for(;(probe->next)!=NULL;probe = probe->next)
			;
		(probe -> next)=&(page_arr[page_no]);
	}
}


//从链表中取出一个块 链表为空返回ERRO_POP_BLOCK
uint32_t pop_block(page_c_t c)
{
	pm_page_t * header;
	switch(c)
	{
		case list_1:
			header = buddy_ptr ->list_1;
			if(header==NULL)
				return ERRO_POP_BLOCK;
			if(header->next==NULL){
				buddy_ptr ->list_1 =NULL;
				return header->page_no;
			}
			break;
		case list_2:
			header = buddy_ptr ->list_2;
			if(header==NULL)
				return ERRO_POP_BLOCK;
			if(header->next==NULL){
				buddy_ptr ->list_2 = NULL;
				return header->page_no;
			}
			break;
		case list_4:
			header = buddy_ptr ->list_4;
			if(header==NULL)
				return ERRO_POP_BLOCK;
			if(header->next==NULL){
				buddy_ptr ->list_4 = NULL;
				return header->page_no;
			}
			break;
		case list_8:
			header = buddy_ptr ->list_8;
			if(header==NULL)
				return ERRO_POP_BLOCK;
			if(header->next==NULL){
				buddy_ptr ->list_8 = NULL;
				return header->page_no;
			}
			break;
		case list_16:
			header = buddy_ptr ->list_16;
			if(header==NULL)
				return ERRO_POP_BLOCK;
			if(header->next==NULL){
				buddy_ptr ->list_16 = NULL;
				return header->page_no;
			}
			break;
		case list_32:
			header = buddy_ptr ->list_32;
			if(header==NULL)
				return ERRO_POP_BLOCK;
			if(header->next==NULL){
				buddy_ptr ->list_32 = NULL;
				return header->page_no;
			}
			break;
		case list_64:
			header = buddy_ptr ->list_64;
			if(header==NULL)
				return ERRO_POP_BLOCK;
			if(header->next==NULL){
				buddy_ptr ->list_64 = NULL;
				return header->page_no;
			}
			break;
		case list_128:
			header = buddy_ptr ->list_128;
			if(header==NULL)
				return ERRO_POP_BLOCK;
			if(header->next==NULL){
				buddy_ptr ->list_128 = NULL;
				return header->page_no;
			}
			break;
		case list_256:
			header = buddy_ptr ->list_256;
			if(header==NULL)
				return ERRO_POP_BLOCK;
			if(header->next==NULL){
				buddy_ptr ->list_256 = NULL;
				return header->page_no;
			}
			break;
		case list_512:
			header = buddy_ptr ->list_512;
			if(header==NULL)
				return ERRO_POP_BLOCK;
			if(header->next==NULL){
				buddy_ptr ->list_512 = NULL;
				return header->page_no;
			}
			break;
		case list_1024:
			header = buddy_ptr ->list_1024;
			if(header==NULL)
				return ERRO_POP_BLOCK;
			if(header->next==NULL){
				buddy_ptr ->list_1024 = NULL;
				return header->page_no;
			}
			break;
	}
	pm_page_t * probe = header;
	for(;probe->next->next!=NULL;probe = probe->next)
		;
	uint32_t return_page_no = probe->next->page_no;
	probe->next=NULL;
	return return_page_no;
}


//初始化buddy链表
void init_pmm1()
{
	//取内存结构体数组的首地址
	mmap_entry_t* mmap_start_ptr=(mmap_entry_t*)glb_mboot_ptr->mmap_addr;
	//内存结构体数组的末尾地址
	mmap_entry_t* mmap_end_ptr=(mmap_entry_t*)glb_mboot_ptr->mmap_addr+glb_mboot_ptr->mmap_length;

	mmap_entry_t* mmap_entry;
	//遍历内存结构体数组
	for(mmap_entry=mmap_start_ptr;mmap_entry<mmap_end_ptr;mmap_entry++)
	{	
		//若是1M以上且可用的内存
		if(mmap_entry->type==0x01&&mmap_entry->base_addr_low==0x100000)
		{
			//去掉装载内核文件的一段内存，得到1M以上可用内存起始地址
			pmm_page_start=mmap_entry->base_addr_low+(uint32_t)(kern_end-kern_start);
			pmm_page_size=mmap_entry->base_addr_low+mmap_entry->length_low;
		}
	}
	//初始化64M的物理页
	for(int i=0;i<buddy_page_number;i++)
	{
		//printk(".......%d\n",i);
		page_arr[i].page_no = i;
		page_arr[i].state = 1;
		page_arr[i].next = NULL;
	}
	//装载链表
	append_block(0,list_1);    //1页链表

	append_block(1,list_1);

	append_block(2,list_2);    //2页链表

	append_block(4,list_4);

	append_block(8,list_8);

	append_block(16,list_16);

	append_block(32,list_32);

	append_block(64,list_64);

	append_block(128,list_128);

	append_block(256,list_256);

	append_block(512,list_512);

	append_block(1024,list_1024);

	//剩下的页从大到小全部装载到list中
	int temp_page_no=2048;
	int block_size[11]={1024,512,256,128,64,32,16,8,4,2,1};
	for(int i=0;i<11;i++)
	{
		for(;temp_page_no+block_size[i]<buddy_page_number;temp_page_no+=block_size[i])
		{
			append_block(temp_page_no,list_1024);
		}
	}
}


//获取page编号对应的addr
uint32_t pmm_page_no_to_addr(uint32_t page_no)
{
	return (page_no<<12)+pmm_page_start;
}

//addr转为page_no
uint32_t addr_to_pmm_page_no(uint32_t addr)
{
	return (addr-pmm_page_start)>>12;
}

//将 page_c_t枚举类型转化为对应的块大小 如 (page_c_t)_256——> (uint32_t)256
uint32_t c_to_uint32(page_c_t ph)
{
	uint32_t re = 1;
	re = re<<ph;
	return re;
}

uint32_t get_partner_page_no(uint32_t page_no,page_c_t type)
{
	//--|--|--|--|--|--| 如图 必须整数倍或者0
	//获取内存块占用的页数
	uint32_t v1 = c_to_uint32(type);
	//内存块前面要有两倍的页数
	uint32_t v2 = 2*v1;
	//此时说明page_no前面还有3v1，可以合成一个更大的内存块
	if((page_no - v1)%v2 == 0)
		return page_no-v1;
	else if(page_no % v2 == 0)
		return page_no+v1;
}


uint32_t alloc_helper(page_c_t target_ph,page_c_t pop_ph,uint32_t pop_page_no)
{
	//目标内存块大小，拿到的内存块大小
	for(;pop_ph!=target_ph;pop_ph--)
	{
		//pop_page_no是取出的内存块开始的页编号，(c_to_uint32(pop_ph)/2)是这个内存块占有的页数的一半。
		//append_page_no是分两半后，
		uint32_t append_page_no = pop_page_no+(c_to_uint32(pop_ph)/2);
		//内存块加入到低一级的链表中
		append_block(append_page_no,pop_ph-1);
	}
	return pop_page_no;
}


//伙伴算法多页分配
pm_alloc_t pmm_alloc_pages(uint32_t page_count)
{
	pm_alloc_t return_struct = {0,list_erro,0}; 
	if (page_count>1024||page_count==0)
		return return_struct;    //分配失败
	//来点骚操作
	//获取需要分配的页数
	uint32_t page_count_probe=page_count;
	//获得足够内存块的链表层
	int i=0;
	for(;i<11;i++)
	{
		if(page_count_probe<=(1<<i))
			break;
	}
	//ph为想要的内存块层级
	page_c_t ph = i;
	page_c_t origin_ph = ph;    //存放原始ph
	uint32_t pop_page_no = ERRO_POP_BLOCK;
	//按顺序遍历链表获得内存块
	for(;ph<list_erro;ph++)
	{
		//从链表中获取内存块，获得内存块层级总是大于等于需要的层级
		//返回的是内存块的起始物理页编号，ph也变为了内存块对应的的层级
		pop_page_no = pop_block(ph);
		if(pop_page_no!=ERRO_POP_BLOCK)
			break;
	}

	if(pop_page_no==ERRO_POP_BLOCK){
		return return_struct;    //分配失败
	}
	else
	{
		//将可能获得的大内存块分为需要的小内存块，返回内存块起始页编号
		uint32_t target_page_no = alloc_helper(origin_ph,ph,pop_page_no);
		return_struct.addr = pmm_page_no_to_addr(target_page_no);
		return_struct.state = 1;
		return_struct.size = origin_ph;
		return return_struct;
	}
}


//free页 返回bool型（定义在typs.h中） True-成功free False-失败
void pmm_free_pages(pm_alloc_t block_disc)
{
	//检查页是否在页编号范围内
	uint32_t page_no = addr_to_pmm_page_no(block_disc.addr);
	if(page_no<buddy_page_number)
	{
		page_c_t size = block_disc.size;
		uint32_t partner_page_no = get_partner_page_no(page_no,size);
		//判断size链表中是否有这个内存块的伙伴块
		pm_page_t** header;
		switch(size)
		{
			case list_1:
				header = &(buddy_ptr->list_1);
				break;
			case list_2:
				header = &(buddy_ptr ->list_2);
				break;
			case list_4:
				header = &(buddy_ptr ->list_4);
				break;
			case list_8:
				header = &(buddy_ptr ->list_8);
				break;
			case list_16:
				header = &(buddy_ptr ->list_16);
				break;
			case list_32:
				header = &(buddy_ptr ->list_32);
				break;
			case list_64:
				header = &(buddy_ptr ->list_64);
				break;
			case list_128:
				header = &(buddy_ptr ->list_128);
				break;
			case list_256:
				header = &(buddy_ptr ->list_256);
				break;
			case list_512:
				header = &(buddy_ptr ->list_512);
				break;
			case list_1024:
				header = &(buddy_ptr ->list_1024);
				break;
		}
		if(*header==NULL)
		{
			printk("no buddy\n");
			//没有伙伴可以合并
			append_block(page_no,size);
		}
		else
		{
			//只有一个节点，且是伙伴
			if((*header)->next==NULL)
			{
				if((*header)->page_no==partner_page_no)
				{
					printk("buddy:%d\n",partner_page_no);
					if(page_no<partner_page_no)
						append_block(page_no,size+1);
					else
						append_block(partner_page_no,size+1);
					*header=NULL;
				}
				else
				{
					printk("no buddy\n");
					append_block(page_no,size);
				}
				
			}
			//不止一个节点
			else
			{
				//使用双指针header来指向buddy_ptr->list,这样就可以直接改变buddy_ptr->list的值，将其赋值为空或者其他
				//如果使用单指针header来等于buddy_ptr->list，那么可以修改header指向的值，但是无法修改buddy_ptr->list的值
				//头结点正好是伙伴快
				if((*header)->page_no==partner_page_no)
				{
					printk("buddy:%d\n",partner_page_no);
					if(page_no<partner_page_no)
						append_block(page_no,size+1);
					else
						append_block(partner_page_no,size+1);
					*header=(*header)->next;
				}
				else
				{
					//头结点不是伙伴快
					pm_page_t* prev=*header;
					pm_page_t* next=(*header)->next;
					while(next)
					{
						if(next->page_no==partner_page_no)
						{
							printk("buddy:%d\n",partner_page_no);
							if(page_no<partner_page_no)
								append_block(page_no,size+1);
							else
								append_block(partner_page_no,size+1);
							prev->next=next->next;
							next->next=NULL;
							return;
						}
						else
						{
							prev=next;
							next=next->next;
						}
					}
					printk("no buddy\n");
					append_block(page_no,size);
				}
			}
		}
	}
}


//展示bios提供的物理内存分布
void show_memory_map()
{
	uint32_t mmap_addr=glb_mboot_ptr->mmap_addr;
	uint32_t mmap_length=glb_mboot_ptr->mmap_length;

	printk("memory map:\n");

	//创建一个mmap_entry_t结构体指针，指向mmap_entry_t结构体数组的首地址，用来遍历数组中的所有结构体
	mmap_entry_t* mmap=(mmap_entry_t*)mmap_addr;
	// 遍历结构体数组
	for(; (uint32_t)mmap < mmap_addr +mmap_length; mmap++)
	{
		printk("base_addr = 0x%X%08X, length = 0x%X%08X, type = 0x%X\n",
			(uint32_t)mmap->base_addr_high, (uint32_t)mmap->base_addr_low,
			(uint32_t)mmap->length_high, (uint32_t)mmap->length_low,
			(uint32_t)mmap->type);
	}
}


//初始化物理内存管理
void init_pmm()
{
	//取内存结构体数组的首地址
	mmap_entry_t* mmap_start_ptr=(mmap_entry_t*)glb_mboot_ptr->mmap_addr;
	//内存结构体数组的末尾地址
	mmap_entry_t* mmap_end_ptr=(mmap_entry_t*)glb_mboot_ptr->mmap_addr+glb_mboot_ptr->mmap_length;

	mmap_entry_t* mmap_entry;
	//遍历内存结构体数组
	for(mmap_entry=mmap_start_ptr;mmap_entry<mmap_end_ptr;mmap_entry++)
	{	
		//若是1M以上且可用的内存
		if(mmap_entry->type==0x01&&mmap_entry->base_addr_low==0x100000)
		{
			//去掉装载内核文件的一段内存，得到1M以上可用内存起始地址
			uint32_t page_addr=mmap_entry->base_addr_low+(uint32_t)(kern_end-kern_start);
			uint32_t length=mmap_entry->base_addr_low+mmap_entry->length_low;
			//初始化物理内存页，并将所有页的起始地址放入栈中
			while(page_addr<length&&page_addr<=PMM_MAX_SIZE)
			{
				//将此内存页起始地址放入栈中
				pmm_free_page(page_addr);
				//向后移动4K大小
				page_addr+=PMM_PAGE_SIZE;
				//物理内存页数加1
				phy_page_count++;
			}
		}
	}
	//分配好后，拿出4页物理内存用于buddy结构体
	for(int i=4;i>=1;i--)
	{
		pmm_buddy_free_page(pmm_stack[pmm_stack_top]);
		pmm_stack_top--;
	}
}


//申请内存就是在数组顶部取一个物理页地址
//返回一个内存页的物理地址
uint32_t pmm_alloc_page()
{
	assert(pmm_stack_top!=0,"out of memory");
	//返回栈顶的内存页，此时此页将被分配出去
	uint32_t page=pmm_stack[pmm_stack_top];
	//可用页减1,pmm_stack_top就是数组指针，且从1开始
	pmm_stack_top--;
	return page;
}


uint32_t pmm_buddy_alloc_page()
{
	assert(pmm_buddy_stack_top!=0,"out of memory");
	//返回栈顶的内存页，此时此页将被分配出去
	uint32_t page=pmm_buddy_stack[pmm_buddy_stack_top];
	//可用页减1,pmm_stack_top就是数组指针，且从1开始
	pmm_buddy_stack_top--;
	return page;
}


//释放就是将一个物理页地址放入数组
//释放申请的内存
void pmm_free_page(uint32_t p)
{
	assert(pmm_stack_top!=PAGE_MAX_SIZE,"out of pmm_stack");
	//全局变量初始化为0,先++
	++pmm_stack_top;
	//将此内存页起始地址存放在栈中
	pmm_stack[pmm_stack_top]=p;
}


void pmm_buddy_free_page(uint32_t p)
{
	assert(pmm_buddy_stack_top!=PAGE_MAX_SIZE,"out of pmm_stack");
	//全局变量初始化为0,先++
	++pmm_buddy_stack_top;
	//将此内存页起始地址存放在栈中
	pmm_buddy_stack[pmm_buddy_stack_top]=p;
}
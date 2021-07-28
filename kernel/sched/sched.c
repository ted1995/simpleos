#include "sched.h"
#include "heap.h"
#include "debug.h"


const uint8_t lowest_bitmap[256] =  
{  
    /* 00 */ 0, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0,  
    /* 10 */ 4, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0,  
    /* 20 */ 5, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0,  
    /* 30 */ 4, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0,  
    /* 40 */ 6, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0,  
    /* 50 */ 4, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0,  
    /* 60 */ 5, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0,  
    /* 70 */ 4, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0,  
    /* 80 */ 7, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0,  
    /* 90 */ 4, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0,  
    /* A0 */ 5, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0,  
    /* B0 */ 4, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0,  
    /* C0 */ 6, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0,  
    /* D0 */ 4, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0,  
    /* E0 */ 5, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0,  
    /* F0 */ 4, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0  
};
 
extern uint32_t kern_stack_top;

// 可调度进程链表
struct task_struct *running_proc_head = NULL;

// 等待进程链表
struct task_struct *wait_proc_head = NULL;

// 当前运行的任务
struct task_struct *current = NULL;

//初始执行任务
struct task_struct *init = NULL;

// 空闲任务
struct task_struct *idle = NULL;

//全局优先级队列
extern struct task_struct* pri_queue[max_pri+1];

//一级位图，优先级位图，32个字节，256个bit，对应256个优先级
uint8_t pri_bit[32];

//二级位图，字节位图，4个字节，32个bit，对应一级位图中每一个字节是否为为0
uint8_t byte_bit[4];

//假如优先级只有8个，则只需要一个字节就可以表示8个优先级，此时如果将优先级数组作为一个变量时，一个字节8位的变量可以表示的最大值为11111111=256,最小值00000000=0.
//即当此变量为256时，每一个优先级队列都有就绪的任务，只要事先将256个数对应的首个为1的位记录在一张表中，只要知道变量大小，就可以通过查表获得当前的优先级最高的就绪队列。

//当优先级有32个时，则可以通过一个int变量来表示，4个字节32位，此时可以先查第一个字节，如果第一个字节位为0,就查第二个字节，如果第一个字节不为0,则直接查8位的表，找到对应的
//优先级最高的就绪队列，此时只有一级位图就可以解决。

//当优先级为256时，需要32个字节表示，如果此时依次遍历32个字节，每个字节按照查表法进行，则遍历的时间不可忽略，因此此时使用二级位图。
//32个字节使用4个字节32个位来标识每一个字节的是否为0,




void init_sched(int (*fn)(void *))
{
	printk("kern_stack_top=%x\n",kern_stack_top);

	// 为当前执行流创建信息结构体,如果不创建新任务，当前执行流就直接结束了，
	//因此给当前执行流初始化为了idle任务，如果没有其他任务运行，则运行此idle任务
	current = (struct task_struct *)(kern_stack_top - STACK_SIZE);
	init=current;
	current->state = TASK_RUNNABLE;
	current->pid = now_pid++;
	current->pri=0;
	current->timeslot=0;
	current->cur_time=0;
	current->stack = current; 	// 该成员指向栈低地址
	current->mm = NULL; 		// 内核线程不需要该成员
	current->next = NULL;


	idle=(struct task_struct *)(kern_stack_top  - STACK_SIZE*2);

	idle->state = TASK_RUNNABLE;
	idle->pid = now_pid++;
	idle->pri=0;
	idle->timeslot=0;
	idle->cur_time=0;
	idle->stack = idle; 	// 该成员指向栈低地址
	idle->mm = NULL; 		// 内核线程不需要该成员
	idle->next = NULL;

	//当前线程的栈顶
	uint32_t *stack_top = (uint32_t *)((uint32_t)idle + STACK_SIZE);

	//将相关参数先入栈
	*(--stack_top) = (uint32_t)fn;
	idle->context.esp = (uint32_t)idle + STACK_SIZE - sizeof(uint32_t) ;
	// 设置新任务的标志寄存器未屏蔽中断，很重要
	idle->context.eflags = 0x200;
}


int get_new_task()
{
	//优先级+时间片+两级位图调度
	int non_zero=-1;
	//先遍历二级位图，查表从最低位开始找出第一个不为0的位，代表相应的一级位图第一个不为0的字节
	for(uint8_t j=0;j<4;j++)
	{
		if(byte_bit[j]!=0)
		{
			non_zero=lowest_bitmap[byte_bit[j]]+j*8;
			break;
		}
	}
	if(non_zero==-1)
	{
		return -1;
	}
	//由二级位图找到了第一个不为0的字节，查表获得最高优先级
	return lowest_bitmap[pri_bit[non_zero]]+non_zero*8;
}


void insert_bitmap(uint16_t pri)
{
	//维护一级位图和二级位图
	uint8_t byte_num=pri/8;
	//printk("byte_num=%d\n",byte_num);

	uint8_t bit_num=pri%8;
	//printk("bit_num=%d\n",bit_num);

	//第bytenum个字节，第bitnum个位
	pri_bit[byte_num]|=1<<bit_num;
	//printk("pri=%x\n",pri_bit[byte_num]);

	byte_bit[byte_num/8]|=1<<byte_num%8;

	//printk("pri=%x\n",byte_bit[byte_num]);
}

void delete_bitmap(uint16_t pri)
{
	//如果当前优先级队列已经为空，则更新位图;否则，不用更新。
	if(pri_queue[pri]==NULL)
	{
		//维护一级位图和二级位图
		uint8_t byte_num=pri/8;
		//printk("byte_num=%d\n",byte_num);

		uint8_t bit_num=pri%8;
		//printk("bit_num=%d\n",bit_num);

		//第bytenum个字节，第bitnum个位
		pri_bit[byte_num] &=~(1<<bit_num);
		//printk("pri=%x\n",pri_bit[byte_num]);

		if(pri_bit[byte_num]==0)
		{
			byte_bit[byte_num/8]&=~(1<<byte_num%8);
		}
		//printk("pri=%x\n",byte_bit[byte_num]);
	}
	
}


void schedule()
{
	//用于第一次调度时，将当前执行流切换到idle任务
	if(current==init)
	{
		//printk("first sched\n");
		change_task_to(idle);
		return;
	}
	//当前是idle线程，则寻找是否有新的任务，如果没有就继续idle，如果有就调用新任务
	if(current==idle)
	{
		int highest_pri=get_new_task();
		if(highest_pri==-1)
		{
			//printk("sched -1\n");
			//没有新任务，继续执行idle
			change_task_to(idle);
			return;
		}
		else
		{
			//找到了新任务,取出第一个任务,执行
			//printk("sched %d\n",highest_pri);
			struct task_struct* next=pri_queue[highest_pri];
			pri_queue[highest_pri]=next->next;
			next->next=NULL;

			//任务取出就绪队列，位图也要更新
			delete_bitmap(highest_pri);

			change_task_to(next);
			return;
		}
	}
	//当前不是idle线程
	else
	{
		current->cur_time++;
		//当前时间片没用完
		if(current->cur_time<current->timeslot)
		{
			//printk("timeslot is remain\n");
			//先运行完这个任务
			change_task_to(current);
			return;
		}
		else
		{
			//printk("timeslot is over\n");
			//当前任务时间片用完了
			//先找当前优先级最高的任务
			int highest_pri=get_new_task();
			//printk("highest=%d\n",highest_pri);
			if(highest_pri==-1)
			{
				//没有新任务，继续执行idle
				//先将现在的任务放到就绪队列里，等待下次调度
				pri_queue[current->pri]=current;
				current->next=NULL;
				current->cur_time=0;
				insert_bitmap(current->pri);
				//再调用idle任务
				change_task_to(idle);
				return;
			}
			else
			{
				//找到了新任务,取出第一个任务,执行
				struct task_struct* next=pri_queue[highest_pri];
				//printk("next=%x\n",next);
				//printk("next->next=%x\n",next->next);
				pri_queue[highest_pri]=next->next;
				next->next=NULL;
				//printk("pri_bit=%d\n",pri_bit[highest_pri/8]);
				delete_bitmap(highest_pri);
				//printk("pri_bit=%d\n",pri_bit[highest_pri/8]);

				//如果当前任务未结束,将当前任务放入就绪队列，等待下次调度
				if(current->state!=TASK_ZOMBIE)
				{
					struct task_struct* tmp=pri_queue[current->pri];
					if(tmp==NULL)
					{
						insert_bitmap(current->pri);
						//printk("pri_bit=%d\n",pri_bit[current->pri/8]);
						pri_queue[current->pri]=current;
						current->next=NULL;
						current->cur_time=0;
					}
					else
					{
						while(tmp->next!=NULL)
						{
							tmp=tmp->next;
						}
						tmp->next=current;
						current->next=NULL;
						current->cur_time=0;
					}
				}
				else
				{
					bfree((uint32_t*)current);
				}
				change_task_to(next);
				return;
			}
		}
	}
	
	//直接从优先级链表中，取出当前最高的就绪优先级队列。


	/*
	if(current!=idle)
	{
		//优先级加时间片轮转
		current->cur_time++;
		//当前进程的时间片没用完
		if(current->cur_time<current->timeslot)
		{	
			change_task_to(current);
			return;
		}
		else
		{
			//将此进程时间片清零，等待下次调度。
			current->cur_time=0;
			//遍历优先级队列，寻找新的调度任务,
			for(uint8_t i=0;i<256;i++)
			{
				struct task_struct* next=pri_queue[i];
				while(next!=NULL)
				{
					if(next!=current)
					{
						change_task_to(next);
						return;
					}
					next=next->next;
				}
			}
			//没找到新任务，则切换为idle任务
			change_task_to(idle);
		}
	}
	//当前任务是空闲任务，则寻找有没有新的任务
	else
	{
		//遍历优先级队列，寻找新的调度任务,
		for(uint16_t i=0;i<256;i++)
		{
			struct task_struct* next=pri_queue[i];
			while(next!=NULL)
			{
				if(next!=current)
				{
					change_task_to(next);
					return;
				}
				next=next->next;
			}
		}
		change_task_to(current);
	}
	*/
	
	/*
	//时间片轮转
	if(current)
	{
		if(current&&current->cur_time<current->timeslot)
		{
			current->cur_time++;
			change_task_to(current);
		}
		else
		{
			current->cur_time=0;
			change_task_to(current->next);
		}
	}
	*/	
}

void change_task_to(struct task_struct *next)
{
	
	if (current != next) 
	{
		//printk("current=%x\n",current);
		//printk("next=%x\n",next);
		//printk("current != next\n");
		struct task_struct *prev = current;
		current = next;
		//上下文切换
		switch_to(&(prev->context), &(current->context));	
	}
}
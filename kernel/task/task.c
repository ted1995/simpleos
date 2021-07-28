#include "gdt.h"
#include "pmm.h"
#include "vmm.h"
#include "heap.h"
#include "task.h"
#include "sched.h"
#include "string.h"
#include "debug.h"
#include "timer.h"

//一级位图，优先级位图，32个字节，256个bit，对应256个优先级
extern uint8_t pri_bit[32];

//二级位图，字节位图，4个字节，32个bit，对应一级位图中每一个字节是否为为0
extern uint8_t byte_bit[4];

// 全局 pid 值
pid_t now_pid = 0;

//全局优先级数组
struct task_struct* pri_queue[max_pri+1];

// 内核线程创建
int32_t kernel_thread(int (*fn)(void *), void *arg,uint8_t pri,uint8_t timeslot)
{
	//给线程栈分配内存，线程控制块在栈的底部

	struct task_struct *new_task = (struct task_struct *)bmalloc(STACK_SIZE);
	printk("new_task=%x\n",new_task);
	
	assert(new_task != NULL, "kern_thread: kmalloc error");

	// 将栈低端线程控制块初始化为 0 
	bzero(new_task, sizeof(struct task_struct));
	//设置线程控制块参数
	new_task->state = TASK_RUNNABLE;
	new_task->stack = current;
	new_task->pid = now_pid++;
	new_task->timeslot=timeslot;
	new_task->cur_time=0;
	new_task->mm = NULL;
	new_task->pri=pri>255?255:pri;
	new_task->next=NULL;

	//插入优先级队列
	if(pri_queue[pri]==NULL)
	{
		//空闲队列插入任务，维护一级位图和二级位图
		insert_bitmap(pri);
		pri_queue[pri]=new_task;
	}
	else
	{
		new_task->next=pri_queue[pri]->next;
		pri_queue[pri]->next=new_task;
	}


	//当前线程的栈顶
	uint32_t *stack_top = (uint32_t *)((uint32_t)new_task + STACK_SIZE);

	//将相关参数先入栈
	*(--stack_top) = (uint32_t)arg;
	*(--stack_top) = (uint32_t)kthread_exit;
	*(--stack_top) = (uint32_t)fn;

	//设置新任务的esp，esp指向待执行函数fn，
	//当执行switch——to函数时，会将此esp的值赋给esp寄存器，然后执行ret语句，
	//ret会自动从当前栈顶也就是esp处弹出要执行的下一条语句的地址到eip寄存器，执行代码，恰好执行的是fn函数
	new_task->context.esp = (uint32_t)new_task + STACK_SIZE - sizeof(uint32_t) * 3;

	//详细解释构建新任务的栈分布
	//当前线程栈顶stack_top
	//arg（传入的参数）
	//kthread_exit（线程退出函数）
	//fn（线程执行的函数）（esp）
	//
	//
	//中间是空闲的一段内存
	//
	//
	//线程PCB块占据一段内存
	//当前线程内存起始地址new_task（低地址）


	// 设置新任务的标志寄存器未屏蔽中断，很重要
	new_task->context.eflags = 0x200;

	return new_task->pid;
}

void kthread_exit()
{
	//读取eax的值
	register uint32_t val asm ("eax");
	
	printk("Thread exited with value %d\n", val);

	//从就绪队列中删除此任务
	current->state=TASK_ZOMBIE;
	while(1);
}


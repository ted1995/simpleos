#include "idt.h"
#include "string.h"
#include "debug.h"
#include "vmm.h"
#include "pmm.h"

// 采用两级页表机制

// 内核页目录,本质是一个数组，数组内每个元素4个字节，作为一个页目录项
//__attribute__设置属性，数组按4k对齐
pgd_t pgd_kern[PGD_SIZE] __attribute__ ((aligned(PAGE_SIZE)));

// 内核页表，本质是一个二维数组，128个页表，每个页表中1024个页表项，一个页表项对应一个4k的页
static pte_t pte_kern[PTE_COUNT][PTE_SIZE] __attribute__ ((aligned(PAGE_SIZE)));


void init_vmm()
{
	//页目录中有1024个页目录项，共可以映射4G的内存，一个页目录项对应一个页表，512M内存映射128个页表只占用128个目录项
	// 0xC0000000 (3G）这个地址在页目录中的索引
	uint32_t kern_pte_first_idx = PGD_INDEX(PAGE_OFFSET);
	
	uint32_t i, j;

	//初始化页目录，将128个页表的地址放入3G地址起对应的页目录项中，
	for (i = kern_pte_first_idx, j = 0; i < PTE_COUNT + kern_pte_first_idx; i++, j++) {
		// 此处是内核虚拟地址，MMU 需要物理地址，所以减去偏移，下同
		//pgd_kern[i]是第i个页目录项，存放的是第j个页表的地址，pte_kern[j]是第j个数组的地址，是第j个页表的页表地址
		//因为4k对齐，低12位用不到,因此可以存放一些标志位
		pgd_kern[i] = ((uint32_t)pte_kern[j] - PAGE_OFFSET) | PAGE_PRESENT | PAGE_WRITE;
	}


	uint32_t *pte = (uint32_t *)pte_kern;
	// 不映射第 0 页，便于跟踪 NULL 指针
	//初始化页表，一共有128×1024个页，从1开始编号，按4k对齐，作为页表项放入页表中
	//0<<12=0M,1024*128<<12=512M,所以将页表初始化为对应0～512M物理内存的4k大小的页地址
	for (i = 1; i < PTE_COUNT * PTE_SIZE; i++) {
		pte[i] = (i << 12) | PAGE_PRESENT | PAGE_WRITE;
	}

	//页目录存放的物理地址
	uint32_t pgd_kern_phy_addr = (uint32_t)pgd_kern - PAGE_OFFSET;


	//将页目录的物理地址放入cr3寄存器，cr3是页目录物理基址寄存器
	switch_pgd(pgd_kern_phy_addr);
}

void switch_pgd(uint32_t pd)
{
	asm volatile ("mov %0, %%cr3" : : "r" (pd));
}


//将物理地址pa映射到虚拟地址va
void map(pgd_t *pgd_now, uint32_t va, uint32_t pa, uint32_t flags)
{ 	
	//获取虚拟地址的页目录偏移
	uint32_t pgd_idx = PGD_INDEX(va);
	//获取虚拟地址的页表偏移
	uint32_t pte_idx = PTE_INDEX(va); 
	//获取页表地址
	pte_t *pte = (pte_t *)(pgd_now[pgd_idx] & PAGE_MASK);
	//页表地址为空，说明未给页表分配内存页
	if (!pte) 
	{
		//从物理内存页中取一个空白页出来
		pte = (pte_t *)pmm_alloc_page();
		//此空白页作为一个页表，并将此页表物理地址存入此虚拟地址对应的目录项中
		pgd_now[pgd_idx] = (uint32_t)pte | PAGE_PRESENT | PAGE_WRITE;

		// 将此物理地址转换成3G以上的虚拟地址
		pte = (pte_t *)((uint32_t)pte + PAGE_OFFSET);
		//清空虚拟地址上的此页
		bzero(pte, PAGE_SIZE);
	} 
	//页表地址不为空
	else 
	{
		// 将此物理地址转换到虚拟地址
		pte = (pte_t *)((uint32_t)pte + PAGE_OFFSET);
	}

	//将物理地址存入页表项
	pte[pte_idx] = (pa & PAGE_MASK) | flags;

	// 通知 CPU 更新页表缓存
	asm volatile ("invlpg (%0)" : : "a" (va));
}



//取消虚拟地址的映射
void unmap(pgd_t *pgd_now, uint32_t va)
{
	uint32_t pgd_idx = PGD_INDEX(va);
	uint32_t pte_idx = PTE_INDEX(va);

	pte_t *pte = (pte_t *)(pgd_now[pgd_idx] & PAGE_MASK);

	if (!pte) {
		return;
	}

	// 转换到内核线性地址
	pte = (pte_t *)((uint32_t)pte + PAGE_OFFSET);

	pte[pte_idx] = 0;

	// 通知 CPU 更新页表缓存
	asm volatile ("invlpg (%0)" : : "a" (va));
}


//获取va映射的物理地址并存放到pa中
uint32_t get_mapping(pgd_t *pgd_now, uint32_t va, uint32_t *pa)
{
	uint32_t pgd_idx = PGD_INDEX(va);
	uint32_t pte_idx = PTE_INDEX(va);

	pte_t *pte = (pte_t *)(pgd_now[pgd_idx] & PAGE_MASK);
	if (!pte) {
	      return 0;
	}
	
	// 转换到内核线性地址
	pte = (pte_t *)((uint32_t)pte + PAGE_OFFSET);

	// 如果地址有效而且指针不为NULL，则返回地址
	if (pte[pte_idx] != 0 && pa) {
		 *pa = pte[pte_idx] & PAGE_MASK;
		return 1;
	}

	return 0;
}

// SPDX-License-Identifier: GPL-2.0
#include <linux/fs.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/mm.h>
#include <linux/hugetlb.h>
#include <linux/mman.h>
#include <linux/mmzone.h>
#include <linux/proc_fs.h>
#include <linux/percpu.h>
#include <linux/quicklist.h>
#include <linux/seq_file.h>
#include <linux/swap.h>
#include <linux/vmstat.h>
#include <linux/atomic.h>
#include <linux/vmalloc.h>
#ifdef CONFIG_CMA
#include <linux/cma.h>
#endif
#include <asm/page.h>
#include <asm/pgtable.h>
#include "internal.h"

void __attribute__((weak)) arch_report_meminfo(struct seq_file *m)
{
}

static void show_val_kb(struct seq_file *m, const char *s, unsigned long num)
{
	seq_put_decimal_ull_width(m, s, num << (PAGE_SHIFT - 10), 8);
	seq_write(m, " kB\n", 4);
}

static int meminfo_proc_show(struct seq_file *m, void *v)
{
	struct sysinfo i;
	unsigned long committed;
	long cached;
	long available;
	unsigned long pages[NR_LRU_LISTS];
	unsigned long sreclaimable, sunreclaim;
	int lru;

	si_meminfo(&i); //刷数据到struct sysinfo中
	si_swapinfo(&i);
	committed = percpu_counter_read_positive(&vm_committed_as);

	cached = global_node_page_state(NR_FILE_PAGES) - total_swapcache_pages() - i.bufferram;
	if (cached < 0)
		cached = 0;

	for (lru = LRU_BASE; lru < NR_LRU_LISTS; lru++)
		pages[lru] = global_node_page_state(NR_LRU_BASE + lru);

	available = si_mem_available();
	sreclaimable = global_node_page_state(NR_SLAB_RECLAIMABLE);
	sunreclaim = global_node_page_state(NR_SLAB_UNRECLAIMABLE);

	//系统当前可用的物理内存总量，通过读取全局变量_totalram_pages获取
	show_val_kb(m, "MemTotal:       ", i.totalram);
	//系统当前空闲物理内存，通过读取全局变量vm_zone_stat[]中的NR_FREE_PAGES来获得
	show_val_kb(m, "MemFree:        ", i.freeram);
	//系统中可用的页面数量，公式为Available = memfree + pagecache + reclaimable(可回收页面) − totalreserve_pages(系统保留页面)
	show_val_kb(m, "MemAvailable:   ", available);
	//用于块层的缓存
	show_val_kb(m, "Buffers:        ", i.bufferram);
	//用于页面高速缓存的页面。计算公式为Cached = NR_FILE_PAGES – swap_cache(匿名页的swap) − Buffers(块层的缓冲)
	show_val_kb(m, "Cached:         ", cached);
	//匿名页的swap
	show_val_kb(m, "SwapCached:     ", total_swapcache_pages());
	//活跃页面数量
	show_val_kb(m, "Active:         ", pages[LRU_ACTIVE_ANON] + pages[LRU_ACTIVE_FILE]);
	//不活跃页面数量
	show_val_kb(m, "Inactive:       ", pages[LRU_INACTIVE_ANON] + pages[LRU_INACTIVE_FILE]);
	//活跃匿名页
	show_val_kb(m, "Active(anon):   ", pages[LRU_ACTIVE_ANON]);
	//不活跃匿名页
	show_val_kb(m, "Inactive(anon): ", pages[LRU_INACTIVE_ANON]);
	//活跃文件页
	show_val_kb(m, "Active(file):   ", pages[LRU_ACTIVE_FILE]);
	//不活跃文件页
	show_val_kb(m, "Inactive(file): ", pages[LRU_INACTIVE_FILE]);
	//不可回收页
	show_val_kb(m, "Unevictable:    ", pages[LRU_UNEVICTABLE]);
	//不会被交换到交换分区的页
	show_val_kb(m, "Mlocked:        ", global_zone_page_state(NR_MLOCK));

//这部分没有,ARM64上没有ZONE_HIGHMEM
#ifdef CONFIG_HIGHMEM
	show_val_kb(m, "HighTotal:      ", i.totalhigh);
	show_val_kb(m, "HighFree:       ", i.freehigh);
	show_val_kb(m, "LowTotal:       ", i.totalram - i.totalhigh);
	show_val_kb(m, "LowFree:        ", i.freeram - i.freehigh);
#endif

//这部分没有打印，CONFIG_MMU没有使能，会不会有问题？
#ifndef CONFIG_MMU
	show_val_kb(m, "MmapCopy:       ", (unsigned long)atomic_long_read(&mmap_pages_allocated));
#endif

	//交换分区的大小
	show_val_kb(m, "SwapTotal:      ", i.totalswap);
	//交换分区空闲空间大小
	show_val_kb(m, "SwapFree:       ", i.freeswap);
	//脏页的数量，由全局的vm_node_stat[]中的NR_FILE_DIRTY来统计
	show_val_kb(m, "Dirty:          ", global_node_page_state(NR_FILE_DIRTY));
	//正在回写的页面数量，由全局的vm_node_stat[]中的NR_WRITEBACK来统计
	show_val_kb(m, "Writeback:      ", global_node_page_state(NR_WRITEBACK));
	//统计有反向映射（RMAP）的页面，通常这些页面都是匿名页面并且都映射到了用户空间，但是并不是所有匿名页面都配置了反向映射，如部分的shmem和tmpfs页面就没有设置反向映射
	//这个计数由全局的vm_node_stat[ ]中的NR_ANON_MAPPED来统计
	show_val_kb(m, "AnonPages:      ", global_node_page_state(NR_ANON_MAPPED));
	//统计所有"映射到用户地址空间"的文件页
	show_val_kb(m, "Mapped:         ", global_node_page_state(NR_FILE_MAPPED));
	//共享内存（基于tmpfs实现的shmem、devtmfs等）页面的数量，由全局的vm_node_stat[]中的NR_SHMEM来统计
	show_val_kb(m, "Shmem:          ", i.sharedram);
	//内核可回收的内存，包括可回收的slab页面(NR_SLAB_RECLAIMABLE)和其他的可回收的内核页面(NR_KERNEL_MISC_RECLAIMABLE)
	show_val_kb(m, "KReclaimable:   ", sreclaimable + global_node_page_state(NR_KERNEL_MISC_RECLAIMABLE));
	//所有slab页面，包括可回收的slab页面(NR_SLAB_RECLAIMABLE)和不可回收的slab页面(NR_SLAB_UNRECLAIMABLE)
	show_val_kb(m, "Slab:           ", sreclaimable + sunreclaim);
	//可回收的slab页面（NR_SLAB_RECLAIMABLE）
	show_val_kb(m, "SReclaimable:   ", sreclaimable);
	//不可回收的slab页面（NR_SLAB_UNRECLAIMABLE）
	show_val_kb(m, "SUnreclaim:     ", sunreclaim);
	//所有进程内核栈的总大小，由全局的vm_zone_stat[]中的NR_KERNEL_STACK_KB来统计
	seq_printf(m, "KernelStack:    %8lu kB\n", global_zone_page_state(NR_KERNEL_STACK_KB));
	//所有用于页表的页面数量，由全局的vm_zone_stat[]中的NR_PAGETABLE来统计
	show_val_kb(m, "PageTables:     ", global_zone_page_state(NR_PAGETABLE));

//没有使能
#ifdef CONFIG_QUICKLIST
	show_val_kb(m, "Quicklists:     ", quicklist_total_size());
#endif

	//在NFS中，发送到服务器端但是还没有写入磁盘的页面（NR_UNSTABLE_NFS）
	show_val_kb(m, "NFS_Unstable:   ", global_node_page_state(NR_UNSTABLE_NFS));
	//arm64应该不涉及
	show_val_kb(m, "Bounce:         ", global_zone_page_state(NR_BOUNCE));
	//回写过程中使用的临时缓存（NR_WRITEBACK_TEMP）
	show_val_kb(m, "WritebackTmp:   ", global_node_page_state(NR_WRITEBACK_TEMP));
	//...
	show_val_kb(m, "CommitLimit:    ", vm_commit_limit());
	show_val_kb(m, "Committed_AS:   ", committed);
	//vmalloc区域的总大小，也就是虚拟地址空间的大小
	seq_printf(m, "VmallocTotal:   %8lu kB\n", (unsigned long)VMALLOC_TOTAL >> 10);
	//已经使用的vmalloc区域总大小
	show_val_kb(m, "VmallocUsed:    ", 0ul);
	show_val_kb(m, "VmallocChunk:   ", 0ul);
	//percpu机制使用的页面，由pcpu_nr_pages()函数来统计
	show_val_kb(m, "Percpu:         ", pcpu_nr_pages());

#ifdef CONFIG_MEMORY_FAILURE //没有使能
	seq_printf(m, "HardwareCorrupted: %5lu kB\n", atomic_long_read(&num_poisoned_pages) << (PAGE_SHIFT - 10));
#endif

#ifdef CONFIG_TRANSPARENT_HUGEPAGE //没有使能
	//统计透明巨页的数量
	show_val_kb(m, "AnonHugePages:  ", global_node_page_state(NR_ANON_THPS) * HPAGE_PMD_NR);
	//统计在shmem或者tmpfs中使用的透明巨页的数量
	show_val_kb(m, "ShmemHugePages: ", global_node_page_state(NR_SHMEM_THPS) * HPAGE_PMD_NR);
	//使用透明巨页并且映射到用户空间的shmem或者tmpfs的页面数量
	show_val_kb(m, "ShmemPmdMapped: ", global_node_page_state(NR_SHMEM_PMDMAPPED) * HPAGE_PMD_NR);
#endif

#ifdef CONFIG_CMA
	//CMA机制使用的内存
	show_val_kb(m, "CmaTotal:       ", totalcma_pages);
	//CMA机制中空闲的内存
	show_val_kb(m, "CmaFree:        ", global_zone_page_state(NR_FREE_CMA_PAGES));
#endif

	hugetlb_report_meminfo(m);

	arch_report_meminfo(m);

	return 0;
}

static int __init proc_meminfo_init(void)
{
	proc_create_single("meminfo", 0, NULL, meminfo_proc_show);
	return 0;
}
fs_initcall(proc_meminfo_init);



![](./buddy%20principle.png)

__alloc_pages_nodemask是伙伴系统申请页面的主函数，调用路径：
```c
alloc_pages(gfp_mask, order)
alloc_pages_node(numa_node_id(), gfp_mask, order)
__alloc_pages_node(nid, gfp_mask, order);
__alloc_pages(gfp_mask, order, nid);
__alloc_pages_nodemask(gfp_mask, order, preferred_nid, NULL);
```
函数内部有两个逻辑，先尝试走快速分配路径，如果分配不出来，则走慢速路径

## 1、快速分配路径
```c
//快速路径
get_page_from_freelist(alloc_mask, order, alloc_flags, &ac);

//1、遍历所有zone
for_next_zone_zonelist_nodemask(zone, z, ac->zonelist, ac->high_zoneidx, ac->nodemask)
//2、判断这个zone能否在水位之上分配order这么多内存，能就返回true
    zone_watermark_fast(zone, order, mark, ac_classzone_idx(ac), alloc_flags)
//3、真正从buddy分配内存
    page = rmqueue(ac->preferred_zoneref->zone, zone, order, gfp_mask, alloc_flags, ac->migratetype);
//4、如果步骤2返回失败，尝试回收一部分内存，如果回收成功则继续在当前zone分配
//该尝试回收的机制需要开启node_reclaim_mode，但是默认关闭，所以dji系统直接try next zone
    node_reclaim(zone->zone_pgdat, gfp_mask, order);
```

步骤2 详解
| 优先级 | 用户mask | 分配行为 |
| ------ | -------- | ------- |
| 正常   | 如GFP_KERNEL或者GFP_USER等分配 | 不能访问系统中预留的内存，可以使用min的0/8 |
| 高 ALLOC_HIGH     | __GFP_HIGH | 表示这是一次优先级比较高的分配行为。可以使用min的4/8 |
| 艰难 ALLOC_HARDER   | __GFP_ATOMIC或者实时进程 | 表示需要分配页面的进程不能睡眠并且优先级比较高。可以使用min的5/8 |
| OOM ALLOC_OOM   | 若线程组有线程被OOM进程终止，就适当做补偿。 | 用于补偿OOM进程或者线程。可以使用min的6/8 |
| 紧急 ALLOC_NO_WATERMARKS |  __GFP_MEMALLOC或者进程设置了PF_MEMALLOC标志位 | 可以使用min的8/8 |

从这个趋势看得出来，随着紧急程度的增加，可以使用min水线之下的内存比例越来越多


```c
//1、首先对order=0的页做快速检查，因为这个路径是最频繁的。比如我们vmalloc 10个页，底层是调用了10次alloc_page
//剩余内存页数 > 水线 + "z->lowmem_reserve[classzone_idx]
//lowmem_reserve。它是每个zone预留的内存，为了防止高端zone在没内存的情况下过度使用低端zone的内存资源
	if (!order && (free_pages - cma_pages) > mark + z->lowmem_reserve[classzone_idx])
		return true;

//2、如果order>0，或者在这里没申请到页面，进入__zone_watermark_ok函数

//3、根据上表，调整min水位
	if (alloc_flags & ALLOC_HIGH)
		min -= min / 2;

	if (likely(!(alloc_flags & (ALLOC_HARDER|ALLOC_OOM)))) {
		free_pages -= z->nr_reserved_highatomic;
	} else {
		if (alloc_flags & ALLOC_OOM)
			min -= min / 2;
		else
			min -= min / 4;
	}

//4、好，现在知道分配完还有多少free_pages，也知道min水线是多少，那么就把两者进行比较
//注意：这里的free_pages是已经扣除要分配大小的剩余free_pages
	if (free_pages <= min + z->lowmem_reserve[classzone_idx])
		return false;

//5、走过4之后，我们知道zone的free_page大小是够的，但这就代表可以分配了吗？no no no
//一个zone中有几种类型的page，如果不可移动、可移动、可回收的，需要检查下是不是有一种类型 可以完整的分配2^order个page
	for (o = order; o < MAX_ORDER; o++) {
        //...
		for (mt = 0; mt < MIGRATE_PCPTYPES; mt++) {
			if (!list_empty(&area->free_list[mt]))
				return true;
		}
        //...
    }
    return false;
```

步骤3 详解
```c
//1、首先对于order=0的特殊处理，有内存池+无需拿锁，所以速度快
	//从Per-CPU变量per_cpu_pages（PCP机制）中分配物理页面
	//每个zone中 每个CPU都有一个本地per_cpu_pages，保存单页面的链表。这样分配效率高，减少对zone中锁的操作
	if (likely(order == 0)) {
		page = rmqueue_pcplist(preferred_zone, zone, order, gfp_flags, migratetype, alloc_flags);
		goto out;
	}

//2、核心分配逻辑
    do {

    } while(xxx);
//3、__rmqueue_smallest实际执行分配，是一个切蛋糕的逻辑

//4、expand，给出一个示例

```


## 2、慢速分配路径
当快速路径分配失败时，进入慢速路径
page = __alloc_pages_slowpath(alloc_mask, order, &ac);

慢速路径的过程信息用`struct alloc_context *ac`保存，Linux经常用struct这种模式记录信息，不容易搞乱
```c
struct alloc_context {
	struct zonelist *zonelist; //我理解是把一个node中的zone穿成链表
	nodemask_t *nodemask; //内存节点的掩码，是不是说在哪个node上面分配
	struct zoneref *preferred_zoneref; //node中首选的是哪个zone
	int migratetype; //迁移类型
	enum zone_type high_zoneidx; //表示允许分配内存的最高zone
	bool spread_dirty_pages; //用于指定是否传播脏页，不懂
};
```

```c
//1、一些分配过程中的标志位
    //是否允许调用直接页面回收机制
	bool can_direct_reclaim = gfp_mask & __GFP_DIRECT_RECLAIM;
	//表示会形成一定的内存分配压力，当分配的order>3时，置位
	const bool costly_order = order > PAGE_ALLOC_COSTLY_ORDER;

//2、两种mask格式的转换，gfp是对用户的，alloc_flags是内部的
    //因为进入了慢速路径，所以对水位也有所调整；快速路径策略保守，这里激进
	alloc_flags = gfp_to_alloc_flags(gfp_mask);

//3、因为经过快速路径之后环境会变化，重新计算首选zone
	ac->preferred_zoneref = first_zones_zonelist(ac->zonelist, ac->high_zoneidx, ac->nodemask);

//4、唤醒kswapd线程
	if (alloc_flags & ALLOC_KSWAPD)
		wake_all_kswapds(order, gfp_mask, ac);

//5、上面调整水位为min，这里再走一次快速分配路径
	page = get_page_from_freelist(gfp_mask, order, alloc_flags, ac);

//6、尝试内存规整，需要满足以下条件
	//1、允许调用直接页面回收机制
	//2、costly_order，也就是大阶连续内存
	//3、ALLOC_NO_WATERMARKS=0，即不允许访问系统预留内存
    	if (can_direct_reclaim &&
			(costly_order ||
			   (order > 0 && ac->migratetype != MIGRATE_MOVABLE))
			&& !gfp_pfmemalloc_allowed(gfp_mask))

//7、确保kswapd不会进入睡眠，我们再次唤醒它
	if (alloc_flags & ALLOC_KSWAPD)
		wake_all_kswapds(order, gfp_mask, ac);

//8、是否允许使用reserve内存，ALLOC_NO_WATERMARKS=1允许
	reserve_flags = __gfp_pfmemalloc_flags(gfp_mask);

    //如果要使用预留的内存，那么要重新计算首选zone
	if (!(alloc_flags & ALLOC_CPUSET) || reserve_flags) {
		ac->preferred_zoneref = first_zones_zonelist(ac->zonelist,
					ac->high_zoneidx, ac->nodemask);
	}

//9、刚才是调整水线为min，现在额外允许使用reserved内存，所以再尝试一次快速路径分配
	page = get_page_from_freelist(gfp_mask, order, alloc_flags, ac);

//10、上面的方法都失败了，如果不允许直接内存回收，那我们没有其他可以做的了，直接返回失败
	if (!can_direct_reclaim)
		goto nopage;

//11、调用直接内存回收函数，经过一轮直接回收后尝试分配内存
	page = __alloc_pages_direct_reclaim(gfp_mask, order, alloc_flags, ac,
							&did_some_progress);

//12、调用直接内存规整函数，经过一轮直接规整后尝试分配内存
	page = __alloc_pages_direct_compact(gfp_mask, order, alloc_flags, ac,
					compact_priority, &compact_result);

//13、现在我们所有能用的方法都试过了，能做的就是retry；如果不允许retry就返回失败
	if (gfp_mask & __GFP_NORETRY)
		goto nopage;

//14、对于分配高阶内存，如果mask中没有__GFP_RETRY_MAYFAIL，则说明不允许继续重试，返回失败
	if (costly_order && !(gfp_mask & __GFP_RETRY_MAYFAIL))
		goto nopage;

//15、retry回收，里面会判断retry是否有进展
	if (should_reclaim_retry(gfp_mask, order, ac, alloc_flags,
				 did_some_progress > 0, &no_progress_loops))
		goto retry;

	//retry内存规整
	if (did_some_progress > 0 &&
			should_compact_retry(ac, order, alloc_flags,
				compact_result, &compact_priority,
				&compaction_retries))
		goto retry;

//16、所有方法都尝试过了，那么只能使用OOM killer机制杀进程了
	page = __alloc_pages_may_oom(gfp_mask, order, ac, &did_some_progress);
	if (page)
		goto got_pg;

	//如果被杀的是当前进程，那么返回申请失败
	if (tsk_is_oom_victim(current) &&
	    (alloc_flags == ALLOC_OOM ||
	     (gfp_mask & __GFP_NOMEMALLOC)))
		goto nopage;

	/* Retry as long as the OOM killer is making progress */
	//如果刚才OOM之后释放了一些内存，那么retry
	if (did_some_progress) {
		no_progress_loops = 0;
		goto retry;
	}

//17、如果设置了__GFP_NOFAIL表示不能分配失败，那么只能不停的尝试

//18、没设置的话，就返回申请失败了，输出我们经常看见的打印
	//调用warn_alloc()宣告分配失败
	warn_alloc(gfp_mask, ac->nodemask,
			"page allocation failure: order:%u", order);

```


## 3、一个实际的问题
### (1)背景
E3和V1上面将CONFIG_FORCE_MAX_ZONEORDER设置为了14，默认时11，导致在free内存很多的时候，不给pagecache用，很难受

### (2)MIGRATE_HIGHATOMIC
在系统运行一段时间后，会出现大量内存碎片，会导致高阶页块(high-order page)的分配失败。为了避免，减轻这种情况，创建了MIGRATE_HIGHATOMIC类型的页面。在此后的分配中，只有当相同的高阶，并拥有高级分配权限时，才会分配这样的页块。当分配单个页框失败时，这样的页块会被回收，类型发生变化。
```c
enum migratetype {
	MIGRATE_UNMOVABLE,
	MIGRATE_MOVABLE,
	MIGRATE_RECLAIMABLE,
	MIGRATE_PCPTYPES,	/* the number of types on the pcp lists */
	MIGRATE_HIGHATOMIC = MIGRATE_PCPTYPES,
    //...
}
```

### (3)reserve路径——reserve_highatomic_pageblock
在快速分配路径get_page_from_freelist，分配成功后，使用reserve_highatomic_pageblock预留大块内存

### (4)unreserve路径——unreserve_highatomic_pageblock
在慢速路径中的直接内存回收路径__alloc_pages_direct_reclaim中，如果回收后也分配失败，那么使用reserved的内存

### (5)reserve了多少，为什么与MAX_ZONEORDER有关系
```c
static void reserve_highatomic_pageblock(struct page *page, struct zone *zone,
				unsigned int alloc_order)
{
	max_managed = (zone_managed_pages(zone) / 100) + pageblock_nr_pages; //预留的上限
	if (zone->nr_reserved_highatomic >= max_managed)
		return;

	mt = get_pageblock_migratetype(page);
	if (!is_migrate_highatomic(mt) && !is_migrate_isolate(mt) && !is_migrate_cma(mt)) {
		zone->nr_reserved_highatomic += pageblock_nr_pages; //dji maxzoneorder=14申请不到内存的问题
		set_pageblock_migratetype(page, MIGRATE_HIGHATOMIC); //将page类型设置为MIGRATE_HIGHATOMIC
		move_freepages_block(zone, page, MIGRATE_HIGHATOMIC, NULL); //将page的lru成员挂在对应的free_page上面
	}
}
```

预留的上限是怎么算的？
```c
#define MAX_ORDER 11
#define pageblock_order		(MAX_ORDER-1)

//当MAX_ORDER=11，pageblock = pow(2,10)*4/1024 = 4M
//当MAX_ORDER=14，pageblock = pow(2,13)*4/1024 = 32M
#define pageblock_nr_pages	(1UL << pageblock_order)
max_managed = (zone_managed_pages(zone) / 100) + pageblock_nr_pages;
```
也就是说当把当MAX_ORDER配置为14的时候，会有32M内存变成reserve内存，不给pagecache用


## 4、lowmem_reserve[]是什么
看代码的时候发现struct zone中还有lowmem_reserve[MAX_NR_ZONES]，两者都是reserve内存，那这个和zone->nr_reserved_highatomic有什么区别呢？
https://zhuanlan.zhihu.com/p/258560892

低地址内存是更珍贵的，当高地址内存被申请完了之后会占用低地址的内存，而低地址的内存申请却不能使用高地址的内存。举个例子：HIGH_ZONE的内存耗尽之后会使用NORMAL_ZONE的内存，若此时占用了大量NORMAL_ZONE内存，将NORMAL_ZONE也耗尽，且分配时用mlock锁了这些内存(无法被swap)，一直未释放。后续若是HIGH_ZONE的内存得到释放，就导致一个尴尬的局面：HIGH_ZONE有大量空闲内存可用，但是NORMAL_ZONE为了满足HIGH_ZONE的使用致使自己无内存可用，这显然是不合理的。

于是struct zone就需要使用lowmem_reserve[]保留一部分内存不可以被高地址的zone使用，仅供自己使用。具体保留多少由高地址zone的managed pages的大小和一个比例值决定。这个比值位于全局数组sysctl_lowmem_reserve_ratio[MAX_NR_ZONES]中，它有个默认初值，用户还可以通过procfs下的lowmem_reserve_ratio接口来自行修改。如何使用比值计算lowmem_reserve[]大小的代码实现位于setup_per_zone_lowmem_reserve中

简述就是high zone中的内存越大，low zone中为为自己reserve的内存就越多，如果high zone太大的话，high zone甚至完全不能使用low zone的内存。

查看系统中zone的lowmem_reserve[ ]情况：
```
cat /proc/zoneinfo

Node 0, zone      DMA
  per-node stats
.......
      nr_written   86948685
                   67656224
  pages free     3974
        min      2
        low      5
        high     8
        spanned  4095
        present  3995
        managed  3974
        protection: (0, 1851, 64282, 64282, 64282)
```
其中proctection字段中的5个数字就是DMA_ZONE的zone->lowmem_reseve[ ]中的值：0表示自己可以完全使用自己的内存，1851表示保留1851个pages不给DMA32_ZONE使用，64282表示保留64282个pages不给NORMAL_ZONE使用，后面依次是MOVABLE_ZONE和DEVICE_ZONE，至于这两个为什么也是64282是因为这两个zone的managed pages等于0，对着setup_per_zone_lowmem_reserve()中的算法实现很容易就能理解。




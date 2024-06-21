# 采集命令
echo 0 > tracing_on
echo > trace

echo stacktrace > trace_options
echo 1 > events/sched/sched_wakeup/enable
echo 1 > events/sched/sched_switch/enable
echo 1 > tracing_on

sleep 5

cat trace > /blackbox/xxx
echo 0 > tracing_on



# preempt_schedule_common路径
## 用户态CFS  -> 用户态RR 被抢占
     chen_network-1005    [003] d...211   665.129487: sched_switch: prev_comm=chen_network prev_pid=1005 prev_prio=120 prev_state=R+ ==> next_comm=bt_bsa_app next_pid=1080 next_prio=89
     chen_network-1005    [003] d...211   665.129489: <stack trace>
 => __schedule
 => preempt_schedule_common
 => preempt_schedule
 => try_to_wake_up
 => default_wake_function
 => autoremove_wake_function
 => receiver_wake_function
 => __wake_up_common
 => __wake_up_common_lock
 => __wake_up_sync_key
 => sock_def_readable
 => unix_dgram_sendmsg
 => sock_sendmsg
 => __sys_sendto
 => __arm64_sys_sendto
 => invoke_syscall
 => el0_svc_common.constprop.2
 => el0_svc_handler
 => el0_svc

wakeup之后立刻抢占，唤醒路径：
     chen_network-1005    [003] dN..311   665.129474: sched_wakeup: comm=bt_bsa_app pid=1080 prio=89 target_cpu=003
     chen_network-1005    [003] dN..311   665.129478: <stack trace>
 => ttwu_do_wakeup
 => try_to_wake_up
 => default_wake_function
 => autoremove_wake_function
 => receiver_wake_function
 => __wake_up_common
 => __wake_up_common_lock
 => __wake_up_sync_key
 => sock_def_readable
 => unix_dgram_sendmsg
 => sock_sendmsg
 => __sys_sendto
 => __arm64_sys_sendto
 => invoke_syscall
 => el0_svc_common.constprop.2
 => el0_svc_handler
 => el0_svc
 结论：wakeup唤醒

## 内核CFS  -> 内核CFS 被抢占
     ksoftirqd/1-21      [001] d...2..   665.129802: sched_switch: prev_comm=ksoftirqd/1 prev_pid=21 prev_prio=120 prev_state=R+ ==> next_comm=kworker/1:1H next_pid=160 next_prio=100
     ksoftirqd/1-21      [001] d...2..   665.129804: <stack trace>
 => __schedule
 => preempt_schedule_common
 => preempt_schedule
 => migrate_enable
 => _local_bh_enable
 => run_ksoftirqd
 => smpboot_thread_fn
 => kthread
 => ret_from_fork

wakeup路径：
           <...>-112     [000] d...412   665.129778: sched_wakeup: comm=kworker/1:1H pid=160 prio=100 target_cpu=001
           <...>-112     [000] d...412   665.129780: <stack trace>
 => ttwu_do_wakeup      尝试唤醒另一个内核线程，但是怎么没直接抢占呢？
 => try_to_wake_up
 => wake_up_process
 => swake_up_locked
 => complete             完成量
 => mmc_wait_done
 => mmc_request_done
 => sdhci_irq
 => irq_forced_thread_fn 强制中断线程化，即一个内核线程
 => irq_thread
 => kthread
 => ret_from_fork

为什么没有直接被抢占呢？
112是irq/15-mmc0，优先级是49；它唤醒了kworker/1:1H，优先级是100；然后irq/15-mmc0主动放弃了处理器；此时如果还有runnable的rt线程，可以直接运行；但是此时没有，所以去cfs的池里面找一个最适合运行的，这个时候选中了CompositorPrivT这个cfs线程，因为它的vruntimne很小
           <...>-112     [000] d...2..   665.129790: sched_switch: prev_comm=irq/15-mmc0 prev_pid=112 prev_prio=49 prev_state=S ==> next_comm=CompositorPrivT next_pid=505 next_prio=120
           <...>-112     [000] d...2..   665.129791: <stack trace>
 => __schedule
 => schedule
 => irq_thread
 => kthread
 => ret_from_fork

但是被wakeup的kworker/1:1H线程很久没运行了，负载均衡开始工作。cpu0通过一个ipi中断把kworker/1:1H任务推到了cpu1上面，所以kworker/1:1H的栈有smpboot_thread_fn这样类似的函数

总结：rt唤醒了cfs，cfs没有资格抢占rt，所以要过一段时间才能运行；cfs的负载均衡将cfs推到了另一个核，在核间通信的路径上得到了运行


## 用户态CFS  -> 内核态CFS 被抢占
             cat-5016    [002] d...2..   665.133971: sched_switch: prev_comm=cat prev_pid=5016 prev_prio=120 prev_state=R+ ==> next_comm=ksoftirqd/2 next_pid=28 next_prio=120
             cat-5016    [002] d...2..   665.133973: <stack trace>
 => __schedule
 => preempt_schedule_common
 => preempt_schedule
 => migrate_enable
 => rt_spin_unlock
 => __handle_mm_fault
 => handle_mm_fault
 => do_page_fault
 => do_translation_fault
 => do_mem_abort
 => el0_da
 被抢占的任务发生了缺页异常，疑问：唤醒点在哪里呢？
 解答：是__handle_mm_fault函数运行的时候发生了中断，中断唤醒了ksoftirqd/2
             cat-5016    [002] d.L.313   665.133958: sched_wakeup: comm=ksoftirqd/2 pid=28 prio=120 target_cpu=002
             cat-5016    [002] d.L.313   665.133962: <stack trace>
 => ttwu_do_wakeup
 => try_to_wake_up
 => wake_up_process
 => wakeup_softirqd
 => irq_exit
 => __handle_domain_irq
 => gic_handle_irq
 => el1_irq
 => skip_ftrace_call
 => rt_spin_unlock
 => mem_cgroup_commit_charge
 => __handle_mm_fault
 => handle_mm_fault
 => do_page_fault
 => do_translation_fault
 => do_mem_abort
 => el0_da
 结论：中断唤醒发生抢占

## 用户态/内核态 CFS  -> [用户态]CFS 被抢占

     ksoftirqd/3-35      [003] d...2..   665.131000: sched_switch: prev_comm=ksoftirqd/3 prev_pid=35 prev_prio=120 prev_state=R+ ==> next_comm=chen_wireless_mi next_pid=872 next_prio=120
     ksoftirqd/3-35      [003] d...2..   665.131002: <stack trace>
 => __schedule
 => preempt_schedule_common
 => preempt_schedule
 => migrate_enable
 => _local_bh_enable
 => run_ksoftirqd
 => smpboot_thread_fn
 => kthread
 => ret_from_fork

唤醒路径：
     ksoftirqd/3-35      [003] d.L.313   665.130975: sched_wakeup: comm=chen_wireless_mi pid=872 prio=120 target_cpu=003
     ksoftirqd/3-35      [003] d.L.313   665.130977: <stack trace>
 => ttwu_do_wakeup
 => try_to_wake_up
 => wake_up_process
 => process_timeout
 => call_timer_fn
 => run_timer_softirq
 => __do_softirq
 => run_ksoftirqd
 => smpboot_thread_fn
 => kthread
 => ret_from_fork

结论：和上面相同，wakeup之后判断是否需要抢占，这个时候被唤醒线程没资格抢占；于是被push到其他cpu，抢占了那个cpu正在运行的cfs线程

## 数量统计
### 非抢占调度路径
1、asmlinkage __visible                      void __sched schedule(void)      =》 14426个
标准调度路径，ret_to_user就是走这里的
2、void __noreturn                           do_task_dead(void)               =》 13
3、void __sched                              schedule_idle(void)              =》 4913

### 有三个抢占路径
1、static void __sched notrace               preempt_schedule_common(void)    =》  1316个
开抢占时调用
2、asmlinkage __visible void __sched notrace preempt_schedule_notrace(void)   =》  21个
路径比较多，次数少不关注了
3、asmlinkage __visible void __sched         preempt_schedule_irq(void)       =》  2458个
调用时机：el1_irq的最后arm64_preempt_schedule_irq
补充： el0发生中断最后会调用ret_to_user
即内核态发生中断，此时的调度算抢占；用户态发生中断，此时的调度算非抢占

## 结论： 用户态的cfs是可以抢占其他线程(内核or用户态cfs)的，只不过概率比较低



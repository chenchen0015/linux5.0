## 用户态mutex用strace看下
程序如下
```c
#include <stdio.h>
#include <pthread.h>
#include <unistd.h>

pthread_mutex_t mutex;

void* thread_A(void* arg) {
    printf("threadA pid is %d\n", gettid());
    pthread_mutex_lock(&mutex);
    printf("Thread A has acquired the lock\n");
    sleep(5);  // A线程持有锁10秒
    pthread_mutex_unlock(&mutex);
    printf("Thread A has released the lock\n");
    return NULL;
}

void* thread_B(void* arg) {
    printf("threadB pid is %d\n", gettid());
    pthread_mutex_lock(&mutex);
    printf("Thread B has acquired the lock\n");
    // B线程在这里持有锁
    pthread_mutex_unlock(&mutex);
    printf("Thread B has released the lock\n");
    return NULL;
}

int main() {
    pthread_t tid1, tid2;
    pthread_mutex_init(&mutex, NULL);
    
    pthread_create(&tid1, NULL, thread_A, NULL);
    pthread_create(&tid2, NULL, thread_B, NULL);

    pthread_join(tid1, NULL);
    pthread_join(tid2, NULL);

    pthread_mutex_destroy(&mutex);

    printf("Main thread exits\n");

    return 0;
}

```

strace的打印如下：
`strace -o test_strace -t -f ./test`

* 2711695是主线程
* 2711696线程A
* 2711697线程B

```shell
2711695 23:19:09 execve("./test", ["./test"], 0xffffc43224e0 /* 35 vars */) = 0
2711695 23:19:09 brk(NULL)              = 0xaaaae3f50000
2711695 23:19:09 mmap(NULL, 8192, PROT_READ|PROT_WRITE, MAP_PRIVATE|MAP_ANONYMOUS, -1, 0) = 0xffff8f0cb000
2711695 23:19:09 faccessat(AT_FDCWD, "/etc/ld.so.preload", R_OK) = -1 ENOENT (No such file or directory)
2711695 23:19:09 openat(AT_FDCWD, "/etc/ld.so.cache", O_RDONLY|O_CLOEXEC) = 3
2711695 23:19:09 newfstatat(3, "", {st_mode=S_IFREG|0644, st_size=66679, ...}, AT_EMPTY_PATH) = 0
2711695 23:19:09 mmap(NULL, 66679, PROT_READ, MAP_PRIVATE, 3, 0) = 0xffff8f085000
2711695 23:19:09 close(3)               = 0
2711695 23:19:09 openat(AT_FDCWD, "/lib/aarch64-linux-gnu/libc.so.6", O_RDONLY|O_CLOEXEC) = 3
2711695 23:19:09 read(3, "\177ELF\2\1\1\3\0\0\0\0\0\0\0\0\3\0\267\0\1\0\0\0\340u\2\0\0\0\0\0"..., 832) = 832
2711695 23:19:09 newfstatat(3, "", {st_mode=S_IFREG|0755, st_size=1637400, ...}, AT_EMPTY_PATH) = 0
2711695 23:19:09 mmap(NULL, 1805928, PROT_NONE, MAP_PRIVATE|MAP_ANONYMOUS, -1, 0) = 0xffff8eecc000
2711695 23:19:09 mmap(0xffff8eed0000, 1740392, PROT_READ|PROT_EXEC, MAP_PRIVATE|MAP_FIXED|MAP_DENYWRITE, 3, 0) = 0xffff8eed0000
2711695 23:19:09 munmap(0xffff8eecc000, 16384) = 0
2711695 23:19:09 munmap(0xffff8f079000, 48744) = 0
2711695 23:19:09 mprotect(0xffff8f058000, 61440, PROT_NONE) = 0
2711695 23:19:09 mmap(0xffff8f067000, 24576, PROT_READ|PROT_WRITE, MAP_PRIVATE|MAP_FIXED|MAP_DENYWRITE, 3, 0x187000) = 0xffff8f067000
2711695 23:19:09 mmap(0xffff8f06d000, 48744, PROT_READ|PROT_WRITE, MAP_PRIVATE|MAP_FIXED|MAP_ANONYMOUS, -1, 0) = 0xffff8f06d000
2711695 23:19:09 close(3)               = 0
2711695 23:19:09 set_tid_address(0xffff8f0cbf50) = 2711695
2711695 23:19:09 set_robust_list(0xffff8f0cbf60, 24) = 0
2711695 23:19:09 rseq(0xffff8f0cc620, 0x20, 0, 0xd428bc00) = 0
2711695 23:19:09 mprotect(0xffff8f067000, 16384, PROT_READ) = 0
2711695 23:19:09 mprotect(0xaaaab3701000, 4096, PROT_READ) = 0
2711695 23:19:09 mprotect(0xffff8f0d0000, 8192, PROT_READ) = 0
2711695 23:19:09 prlimit64(0, RLIMIT_STACK, NULL, {rlim_cur=8192*1024, rlim_max=RLIM64_INFINITY}) = 0
2711695 23:19:09 munmap(0xffff8f085000, 66679) = 0
2711695 23:19:09 rt_sigaction(SIGRT_1, {sa_handler=0xffff8ef4a700, sa_mask=[], sa_flags=SA_ONSTACK|SA_RESTART|SA_SIGINFO}, NULL, 8) = 0
2711695 23:19:09 rt_sigprocmask(SIG_UNBLOCK, [RTMIN RT_1], NULL, 8) = 0
2711695 23:19:09 mmap(NULL, 8454144, PROT_NONE, MAP_PRIVATE|MAP_ANONYMOUS|MAP_STACK, -1, 0) = 0xffff8e6c0000
2711695 23:19:09 mprotect(0xffff8e6d0000, 8388608, PROT_READ|PROT_WRITE) = 0
2711695 23:19:09 getrandom("\x42\x83\xe5\x59\xe4\xd3\x27\xfd", 8, GRND_NONBLOCK) = 8
2711695 23:19:09 brk(NULL)              = 0xaaaae3f50000
2711695 23:19:09 brk(0xaaaae3f71000)    = 0xaaaae3f71000
2711695 23:19:09 rt_sigprocmask(SIG_BLOCK, ~[], [], 8) = 0
# 创建线程A
2711695 23:19:09 clone(child_stack=0xffff8eece960, flags=CLONE_VM|CLONE_FS|CLONE_FILES|CLONE_SIGHAND|CLONE_THREAD|CLONE_SYSVSEM|CLONE_SETTLS|CLONE_PARENT_SETTID|CLONE_CHILD_CLEARTID, parent_tid=[2711696], tls=0xffff8eecf8e0, child_tidptr=0xffff8eecf1f0) = 2711696
2711695 23:19:09 rt_sigprocmask(SIG_SETMASK, [], NULL, 8) = 0
2711695 23:19:09 mmap(NULL, 8454144, PROT_NONE, MAP_PRIVATE|MAP_ANONYMOUS|MAP_STACK, -1, 0) = 0xffff8deb0000
2711695 23:19:09 mprotect(0xffff8dec0000, 8388608, PROT_READ|PROT_WRITE) = 0
2711695 23:19:09 rt_sigprocmask(SIG_BLOCK, ~[], [], 8) = 0
# 创建线程B
2711695 23:19:09 clone(child_stack=0xffff8e6be960, flags=CLONE_VM|CLONE_FS|CLONE_FILES|CLONE_SIGHAND|CLONE_THREAD|CLONE_SYSVSEM|CLONE_SETTLS|CLONE_PARENT_SETTID|CLONE_CHILD_CLEARTID, parent_tid=[2711697], tls=0xffff8e6bf8e0, child_tidptr=0xffff8e6bf1f0) = 2711697
2711695 23:19:09 rt_sigprocmask(SIG_SETMASK, [], NULL, 8) = 0
# futex init
2711695 23:19:09 futex(0xffff8eecf1f0, FUTEX_WAIT_BITSET|FUTEX_CLOCK_REALTIME, 2711696, NULL, FUTEX_BITSET_MATCH_ANY <unfinished ...>
2711696 23:19:09 rseq(0xffff8eecf8c0, 0x20, 0, 0xd428bc00) = 0
2711696 23:19:09 set_robust_list(0xffff8eecf200, 24) = 0
2711696 23:19:09 rt_sigprocmask(SIG_SETMASK, [], NULL, 8) = 0
2711696 23:19:09 gettid()               = 2711696
2711696 23:19:09 newfstatat(1, "", {st_mode=S_IFCHR|0620, st_rdev=makedev(0x88, 0xd), ...}, AT_EMPTY_PATH) = 0
2711696 23:19:09 mmap(NULL, 134217728, PROT_NONE, MAP_PRIVATE|MAP_ANONYMOUS|MAP_NORESERVE, -1, 0) = 0xffff85eb0000
2711696 23:19:09 munmap(0xffff85eb0000, 34930688) = 0
2711696 23:19:09 munmap(0xffff8c000000, 32178176) = 0
2711696 23:19:09 mprotect(0xffff88000000, 135168, PROT_READ|PROT_WRITE) = 0
2711696 23:19:09 write(1, "threadA pid is 2711696\n", 23) = 23
# 注意这里！！！
# 打印和睡眠之间线程A拿了mutex锁，但是没有调系统调用。说明在unlock的状态拿锁是不需要进入内核的
2711696 23:19:09 write(1, "Thread A has acquired the lock\n", 31) = 31
2711696 23:19:09 clock_nanosleep(CLOCK_REALTIME, 0, {tv_sec=5, tv_nsec=0},  <unfinished ...>
2711697 23:19:09 rseq(0xffff8e6bf8c0, 0x20, 0, 0xd428bc00) = 0
2711697 23:19:09 set_robust_list(0xffff8e6bf200, 24) = 0
2711697 23:19:09 rt_sigprocmask(SIG_SETMASK, [], NULL, 8) = 0
2711697 23:19:09 gettid()               = 2711697
2711697 23:19:09 write(1, "threadB pid is 2711697\n", 23) = 23
# 线程B发现mutex被人持有，需要进入内核将自己加入等锁队列
2711697 23:19:09 futex(0xaaaab3702018, FUTEX_WAIT_PRIVATE, 2, NULL <unfinished ...>
2711696 23:19:14 <... clock_nanosleep resumed>0xffff8eece808) = 0
# 时间过去了5s，线程A放锁。这个时候发现有人在等待，就需要进入内核唤醒等待的人
2711696 23:19:14 futex(0xaaaab3702018, FUTEX_WAKE_PRIVATE, 1 <unfinished ...>
2711697 23:19:14 <... futex resumed>)   = 0
2711696 23:19:14 <... futex resumed>)   = 1
# 此时线程B被唤醒继续执行
2711697 23:19:14 write(1, "Thread B has acquired the lock\n", 31) = 31
# 线程B也放锁，此时没有线程在等这把锁，但是依然进入了内核！！！理论上这里不需要进入内核呀
2711697 23:19:14 futex(0xaaaab3702018, FUTEX_WAKE_PRIVATE, 1) = 0
2711697 23:19:14 write(1, "Thread B has released the lock\n", 31) = 31
2711697 23:19:14 rt_sigprocmask(SIG_BLOCK, ~[RT_1], NULL, 8) = 0
2711697 23:19:14 madvise(0xffff8deb0000, 8314880, MADV_DONTNEED) = 0
2711697 23:19:14 exit(0)                = ?
2711697 23:19:14 +++ exited with 0 +++
2711696 23:19:14 write(1, "Thread A has released the lock\n", 31) = 31
2711696 23:19:14 rt_sigprocmask(SIG_BLOCK, ~[RT_1], NULL, 8) = 0
2711696 23:19:14 madvise(0xffff8e6c0000, 8314880, MADV_DONTNEED) = 0
2711696 23:19:14 exit(0)                = ?
2711695 23:19:14 <... futex resumed>)   = 0
2711696 23:19:14 +++ exited with 0 +++
2711695 23:19:14 write(1, "Main thread exits\n", 18) = 18
2711695 23:19:14 exit_group(0)          = ?
2711695 23:19:14 +++ exited with 0 +++
```






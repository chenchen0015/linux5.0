
# 1、gcc profile （g prof）

## 我的理解
假设有一个函数调用链条A->B->C，在每个函数的头插入一个mcount函数，那么只要记录from_pc，curr_pc，time，就可以获取全部信息了。拿到这些信息，我们就可以知道，某时刻，发生了从函数A到函数B的跳转，有点类似于cpumon的思想


## gcc profiling的工作原理
https://blog.csdn.net/absurd/article/details/1477532


## RISC-V Ftrace 实现原理
这是一个系列，我感觉写的不错
https://tinylab.org/ftrace-impl-1-mcount/


# 2、PGO

## 字节跳动在PGO反馈优化技术上的探索与实践（深度好文）
作者：字节跳动SYSTech
链接：https://juejin.cn/post/7187660471384670267

### 1.原理
PGO(Profile-guided optimization)通常也叫做 FDO(Feedback-directed optimization)，它是一种编译优化技术，它的原理是编译器使用程序的运行时 profiling 信息，生成更高质量的代码，从而提高程序的性能。

传统的编译器优化通常借助于程序的静态分析结果以及启发式规则实现，而在被提供了运行时的 profiling 信息后，编译器可以对应用进行更好的优化。通常来说编译反馈优化能获得 10%-15% 的性能收益，对于特定特征的应用（例如使用编译反馈优化 Clang本身）收益高达 30%。

编译反馈优化通常包括以下手段：

* Inlining，例如函数 A 频繁调用函数 B，B 函数相对小，则编译器会根据计算得出的 threshold 和 cost 选择是否将函数 B inline 到函数 A 中。
* ICP（Indirect call promotion），如果间接调用（Call Register）非常频繁地调用同一个被调用函数，则编译器会插入针对目标地址的比较和跳转指令。使得该被调用函数后续有了 inlining 和更多被优化机会，同时增加了 icache 的命中率，减少了分支预测的失败率。
* Register allocation，编译器能使用运行时数据做更好的寄存器分配。
* Basic block optimization，编译器能根据基本块的执行次数进行优化，将频繁执行的基本块放置在接近的位置，从而优化 data locality，减少访存开销。
* Size/speed optimization，编译器根据函数的运行时信息，对频繁执行的函数选择性能高于代码密度的优化策略。
* Function layout，类似于 Basic block optimization，编译器根据 Caller/Callee 的信息，将更容易在一条执行路径上的函数放在相同的段中。
* Condition branch optimization，编译器根据跳转信息，将更容易执行的分支放在比较指令之后，增加icache 命中率。
* Memory intrinsics，编译器根据 intrinsics 的调用频率选择是否将其展开，也能根据 intrinsics 接收的参数优化 memcpy 等 intrinsics 的实现。

### 2.采样
profile信息有两种
* Instrumentation-based（基于插桩）
* Sample-based（基于采样）

两者对比：（和dji有相通之处）
对比 sampled-based PGO，Instrumentation-based PGO 的优点采集的性能数据较为准确，但繁琐的流程使其在字节跳动业务上难以大规模落地，主要原因有以下几点：

* 应用二进制编译时间长，引入的额外编译流程影响了开发、版本发布的效率。
* 产品迭代速度快，代码更新频繁，热点信息与应用瓶颈变化快。而 instrumented-based PGO 无法使用旧版本收集的 profile 数据编译新版本，需要频繁地使用插桩后的最新版本收集性能数据。
* 插桩引入了额外的性能开销，这些性能开销会影响业务应用的性能特征，收集的 profile 不能准确地表示正常版本的性能特征，从而降低优化的效果，使得 instrumented-based PGO 的优点不再明显。

使用 Sample-based PGO 方案可以有效地解决以上问题：

* 无需引入额外的编译流程，为程序添加额外的调试信息不会明显地降低编译效率。
* Sample-based PGO 对过时的 profile 有一定的容忍性，不会对优化效果产生较大影响。
* 采样引入的额外性能开销很小，可以忽略不计，不会对业务应用的性能特征造成影响。


## Profile Guided Optimizations (PGO) Likely Coming To Linux 5.14 For Clang
https://www.phoronix.com/news/Clang-PGO-For-Linux-Next

## phoronix上面有PGO专题
https://www.phoronix.com/search/Profile%20Guided%20Optimizations


# 3、GCOV
linux对PGO的支持是通过GCOV来实现的，GCOV是一种代码覆盖率采集的工具
CONFIG_GCOV_KERNEL
CONFIG_GCOV_PROFILE_ALL

## gcov代码覆盖率测试-原理和实践总结 (泛而全)
https://blog.csdn.net/yanxiangyfg/article/details/80989680

## 全国大学生计算机系统能力大赛--操作系统比赛  proj65-build-kernel-with-profile-guided-gcc
https://github.com/oscomp/proj65-build-kernel-with-profile-guided-gcc

* 反馈编译优化（Profile-guided optimization，PGO）是一种编译优化技术，通过利用程序的运行时信息指导编译器进行更好的分支选择、函数内联、循环展开等优化，以获取更好的性能。
* Gcov是GCC的覆盖率测试工具，它使用与PGO相同格式的文件记录程序运行时信息。Linux kernel已经支持使用Gcov收集覆盖率信息，因此可以使用该信息反馈式编译优化内核。
* 本实验的目标是对Linux kernel构建进行PGO优化，实现在特定内核上运行Redis的性能提升。主要目标包括两点：
    * 基于Compass CI的构建和benchmark能力，打造gcc反馈式编译优化流程，构建出优化后的linux kernel RPM包。
    * 实现优化后的内核运行Redis的性能提升5%，挑战10%。

总结就是说，内核没直接支持PGO，但是支持了GCOV，然后两者的profile格式是差不多的

## gcc + pgo编译内核实践
https://blog.csdn.net/tianya_lu/article/details/125235253

(1)编译
```
CONFIG_DEBUG_FS=y
CONFIG_GCOV_KERNEL=y
CONFIG_GCOV_PROFILE_ALL=y

sudo make KCFLAGS="-fprofile-dir=/kernel-pgo/"
```
开启CONFIG_GCOV_PROFILE_ALL后一个float2fix函数报错，在这个目录的Makefile中添加`GCOV_PROFILE := n`，控制该目录不使能PGO

(2)测试 拉数据
数据位置在`/sys/kernel/debug/gcov/kernel-pgo/`

(3)回头再编译
```
CONFIG_DEBUG_FS=n
CONFIG_GCOV_KERNEL=n
CONFIG_GCOV_PROFILE_ALL=n

sudo make KCFLAGS="-fprofile-use -fprofile-dir=/kernel-pgo/ -fprofile-correction -Wno-coverage-mismatch -Wno-error=coverage-mismatch"
```

## 内核实践
https://docs.kernel.org/next/translations/zh_CN/dev-tools/gcov.html
内容和上面一个差不多，部分来自于kernel/gcov/Kconfig中的信息


## chad内核实践——机器内拷贝打包
https://docs.kernel.org/translations/zh_CN/dev-tools/gcov.html#gcov-build-zh (官方文档)

拷贝：cp -d -R /sys/kernel/debug/gcov/kernel-pgo ./
压缩 busybox tar -cvf chad.tar kernel-pgo/
解压 tar -xvf chad.tar


## DJI内核开启GCOV/PGO的方法
1、开启编译选项，在defconfig_shell.sh中配置
```shell
./scripts/config -e CONFIG_GCOV_KERNEL
./scripts/config -e CONFIG_GCOV_PROFILE_ALL
./scripts/config -e CONFIG_GCOV_FORMAT_AARCH64_LINUX_ANDROID_GCC_4_9
```

打开CONFIG的本质是打开了下面两个编译选项
```Makefile
CFLAGS_GCOV	:= -fprofile-arcs -ftest-coverage \
	$(call cc-option,-fno-tree-loop-im) \
	$(call cc-disable-warning,maybe-uninitialized,)
export CFLAGS_GCOV
```

2、在linux/Makefile中添加
```Makefile
KBUILD_CFLAGS += -fprofile-dir=/home/chad.chen/kernel-pgo/

#应该改成如下，二次编译的时候才能识别到
#-fprofile-dir=/home/chad.chen/eagle_v1/kernel/linux-ack/
```
注意：这里有坑，为什么不像教程直接指定`-fprofile-dir=/kernel-pgo/`呢？

因为gcov会生成软链接文件，如果按照教程写的话，需要将kernel-pgo文件夹放入服务器根目录下，但是服务器是没有根目录权限的，所以这里要改成家目录下

3、在媒体的驱动中遇到了报错，大意是`-mgeneral-regs-only`编译选项，不使用浮点寄存器，但是嘞`float2fix`这个函数使用了，所以报错。

针对这个问题我们直接在文件夹的Makefile中第一行添加`GCOV_PROFILE := n`，控制该目录不使能PGO

4、之后可以编译成功，然后采集数据，数据在`/sys/kernel/debug/gcov/`中

5、将数据复制出来，然后打包

这一步是有坑的，`.gcno文件`是软链接，没办法直接复制，必须使用-d选项。

对于这个坑，官方文档给了个复杂的脚本，我是没跑起来，我用的如下命令，也可以运行。
```shell
cp -d -R /sys/kernel/debug/gcov/home/chad.chen/kernel-pgo/ ./
busybox tar -cvf chad.tar kernel-pgo/
```

6、将数据复制到服务器上
```shell
tar -xvf chad.tar #解压缩
cp -r kernel-pgo ~/
```
此时，gcov的采样数据就在我们刚刚指定的`/home/chad.chen/kernel-pgo/`中了

7、传入profile数据路径，重新编译

首先关闭这些选项
```shell
./scripts/config -e CONFIG_GCOV_KERNEL
./scripts/config -e CONFIG_GCOV_PROFILE_ALL
./scripts/config -e CONFIG_GCOV_FORMAT_AARCH64_LINUX_ANDROID_GCC_4_9
```

然后在Makefile中添加
```Makefile
KBUILD_CFLAGS += -fprofile-use -fprofile-dir=/home/chad.chen/kernel-pgo/ -fprofile-correction -Wno-coverage-mismatch -Wno-error=coverage-mismatch
```

还需要将Android.mk的第二内核编译取消，变成增量编译，否则会清空linux-ack中生成的*.gcno。注意也不要使用 make clean

最后再次执行编译，注意，如果刚才kernel-pgo没有放对地方，这一步编译会报错，`找不到kernel-pgo/xxx/xxx/xxx.gcda`

8、将生成的固件烧录到机器中验证收益


## gcov的数据如何解析？
背景：目前已经把PGO的流程跑通了，但是看vmlinux的function layout发现函数排布顺序没有变化，所以需要深入分析profile数据


(1)profile的数据格式长什么样？
```
├── kernel
│   ├── async.gcda
│   ├── async.gcno -> /kernel-pgo//kernel/async.gcno
│   ├── async_call.gcda
│   ├── async_call.gcno -> /kernel-pgo//kernel/async_call.gcno
```

修改路径后对应的应该是
```
├── kernel
│   ├── async.gcda
│   ├── async.gcno -> /home/chad.chen/eagle_v1/kernel/linux-ack//kernel/async.gcno
│   ├── async_call.gcda
│   ├── async_call.gcno -> /home/chad.chen/eagle_v1/kernel/linux-ack//kernel/async_call.gcno
```
这样这个软连接就可以真正链接到gcno实体存放的地方了

(2).gcno和.gcda两种格式的数据都代表了什么

根据 chatgpt 的说法

* .gcno：编译时生成
    * .gcno 文件是编译阶段由编译器生成的，它包含了源文件的控制流图信息，用于在运行时分析程序的控制流程。
    * 这些文件提供了程序源文件的控制流图，用于确定哪些代码路径是可执行的，并在执行时收集覆盖率信息。
    * .gcno 文件与编译后的目标文件（例如可执行文件）放置在相同的目录中。
* .gcda：运行时生成
    * .gcda 文件是由程序执行时生成的，它包含了程序在运行过程中的控制流信息以及每个基本块（basic block）的执行次数
    * 这些文件记录了程序执行时每个源文件中哪些代码行被执行了，以及被执行的次数
    * .gcda 文件通常由运行时系统收集，因此必须在程序运行时生成覆盖率数据


(3)kernel生成的.gcno文件可能有问题，是一个软连接，而被链接的地方并没有实体的东西。最后变成一个递归循环

```shell
chad.chen@:linux-ack/kernel-pgo$ ls -ahl kernel/cpumon.gcno
lrwxrwxrwx 1 chad.chen chad.chen 31 May 31 11:25 kernel/cpumon.gcno -> /kernel-pgo//kernel/cpumon.gcno
```

而正确的.gcno里面是（用户态的测试）
```shell
chad.chen@:~/chad_file/gcov$ cat hello.gcno 
oncg*49A��I/home/chad.chen/chad_file/gcov�rp׋)+��mainhello.c
ACCCCCCE        hello.chello.chello.hello.c    
```
我理解里面存放的是每个每个程序的路径，因为最后gcov要给出一个函数行级的layout

前面`修改路径为/home/chad.chen/eagle_v1/kernel/linux-ack/`可以解决这个问题

但是还是有一个问题，服务器上面不能这么搞绝对路径，所以我得把

(4)成功解析gcov数据

将gcda数据拷贝到.c文件所在位置，然后运行`aarch64-linux-android-gcov kernel/cpumon.gcno`，会在linux目录下生成xxx.c.gcov，里面的信息如下

```
     1116: 2974:		} else if (check_in_state(info, KTOP_SAMPLE_CPU)) {
        -: 2975:			static unsigned long long last_cpu_sample_time = 0;
        -: 2976:			unsigned long long sample_time = 0;
      254: 2977:			filtered = i = 0;
        -: 2978:
      254: 2979:			if (info->log_level >= KTOP_LOG_DEBUG)
    #####: 2980:				cur_time = sched_clock();
        -: 2981:
      254: 2982:			read_lock(&tasklist_lock);
```

* 前面是数字代表了一行运行了多少次
* 前面是`#####`代表这一行没有执行到
* `-`表示这一行不可执行


# 4、Others

## if分支预测profile
CONFIG_PROFILE_ALL_BRANCHES: Profile all if conditionals
https://cateee.net/lkddb/web-lkddb/PROFILE_ALL_BRANCHES.html

This tracer profiles all branch conditions. Every if () taken in the kernel is recorded whether it hit or miss. The results will be displayed in:

/sys/kernel/tracing/trace_stat/branch_all

This option also enables the likely/unlikely profiler.

This configuration, when enabled, will impose a great overhead on the system. This should only be enabled when the system is to be analyzed in much detail.

但是dji业务中，branch-miss不是stall的大头，是否要投入研究这个？


## 插眼
一个人分析clang内核 gcov源码：http://eathanq.github.io/2020/12/12/gcov/
美团的实践：https://tech.meituan.com/2018/12/27/ios-increment-coverage.html
生成函数调用图：https://silverrainz.me/blog/gen-graph-from-c-code.html
IT项目研发过程中的利器——C/C++项目调用图篇：https://blog.csdn.net/breaksoftware/article/details/135971707

LTO优化：https://blog.csdn.net/fickyou/article/details/52381776
LTO功能最开始在GCC4.5上出现，但是在4.7上才变得可用。但是它仍然有一些限制，其中一个就是：所有涉及的对象文件都必须使用相同的命令行选项来编译。下面将讲到，这个限制对于kernel来说是一个问题。

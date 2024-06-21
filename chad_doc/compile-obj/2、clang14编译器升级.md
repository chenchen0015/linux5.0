## 背景
做编译器升级（gcc4.9 -> clang14）+开启内核PGO

## 编译器的路径
* gcc 4.9 bin在codebase中的路径是`~/eagle_v1/prebuilts/gcc/linux-x86/aarch64/aarch64-linux-android-4.9/bin/`
* clang14 bin在codebase中的路径是`~/eagle_v1/prebuilts/clang/host/linux-x86/clang-r450784d/bin`，架构在编译的时候指定


## Android.mk中的编译命令
### 1、编译的句根 kernel_MAKE_CMD
```Android.mk
		kernel_MAKE_CMD := +make -C $(LOCAL_PATH) ARCH=$(TARGET_ARCH) \
			CROSS_COMPILE=$(DJI_SOURCE_TOP)/$(CROSS_COMPILE) $(KERNEL_CFLAGS) \
			HOST_COMPILE=$(DJI_SOURCE_TOP)/$(HOST_COMPILE) -j$(BUILD_CORE_NUM)
```

里面的变量都是什么呢？

$(info $(kernel_MAKE_CMD))

打印出来的结果是：

+make -C kernel/linux-ack ARCH=arm64 CROSS_COMPILE=/home/chad.chen/eagle_v1/prebuilts/gcc/linux-x86/aarch64/aarch64-linux-android-4.9/bin/aarch64-linux-androidkernel-  HOST_COMPILE=/home/chad.chen/eagle_v1/prebuilts/gcc/linux-x86/host/x86_64-linux-glibc2.15-4.8/bin/x86_64-linux- -j8

那么推理一下
* LOCAL_PATH => kernel/linux-ack
* TARGET_ARCH => arm64
* DJI_SOURCE_TOP => /home/chad.chen/eagle_v1/
* CROSS_COMPILE => prebuilts/gcc/linux-x86/aarch64/aarch64-linux-android-4.9/bin/aarch64-linux-androidkernel-
* KERNEL_CFLAGS => 空
* HOST_COMPILE => prebuilts/gcc/linux-x86/host/x86_64-linux-glibc2.15-4.8/bin/x86_64-linux-
* BUILD_CORE_NUM => 8


这个部分要修改为
```
		kernel_MAKE_CMD := +make -C $(LOCAL_PATH) ARCH=$(TARGET_ARCH) \
			CC=clang CROSS_COMPILE=aarch64-linux-gnu- \
			LLVM=$(DJI_SOURCE_TOP)/$(LLVM_PATH) -j$(BUILD_CORE_NUM)
```

* (1)CC指定编译器为clang
* (2)指定LLVM后端路径


### 2、实际的编译命令
(1)首先在device/dji/eagle3/eagle3_ac205/BoardConfig.mk中，定义了如下变量
```Makefile
# kernel configurations
TARGET_KERNEL_DIR := kernel/dji/eagle
TARGET_KERNEL_IMAGE := $(TARGET_KERNEL_DIR)/arch/arm/boot/zImage
TARGET_KERNEL_CONFIG := eagle3_defconfig
TARGET_KERNEL_CONFIG_SHELL := device/dji/eagle3/eagle3_ac205/defconfig_shell.sh
CROSS_COMPILE := prebuilts/gcc/linux-x86/aarch64/aarch64-linux-android-4.9/bin/aarch64-linux-androidkernel-
HOST_COMPILE := prebuilts/gcc/linux-x86/host/x86_64-linux-glibc2.15-4.8/bin/x86_64-linux-
```

(2)然后linux-ack/Android.mk中执行了下面的命令
```Makefile
TARGET_IMAGE := Image
KERNEL_IMAGE := $(LOCAL_PATH)/arch/arm64/boot/Image

# 依赖clean 和 .config
$(KERNEL_IMAGE): $(IMAGE_CLEAN) $(LOCAL_PATH)/.config
# 编译第二内核
ifeq ($(TARGET_BOARD_PLATFORM), $(filter $(TARGET_BOARD_PLATFORM), v1 eagle2 e3t eagle3))
	$(hide) echo "Configuring crash kernel with k2_$(TARGET_KERNEL_CONFIG).."
	$(hide) $(kernel_MAKE_CMD) k2_$(TARGET_KERNEL_CONFIG)
	$(hide) echo "Building crash kernel.."
	$(hide) $(kernel_MAKE_CMD) $(TARGET_IMAGE)
	$(hide) $(kernel_CP_KERNEL2)

# 编译第一内核
	$(hide) echo "Configuring kernel with $(TARGET_KERNEL_CONFIG).."
    # TARGET_KERNEL_CONFIG = eagle3_defconfig
	$(hide) $(kernel_MAKE_CMD) $(TARGET_KERNEL_CONFIG)
	$(hide) if [ $(TARGET_KERNEL_CONFIG_SHELL) ];then \
		$(DJI_SOURCE_TOP)/$(TARGET_KERNEL_CONFIG_SHELL); \
	fi

	$(hide) echo "Building kernel.."
	$(hide) $(kernel_MAKE_CMD) $(TARGET_IMAGE)

	$(hide) echo "Building dtbs.."
	$(hide) $(kernel_MAKE_CMD) dtbs

	$(hide) echo "Building kernel modules.."
	$(hide) $(kernel_MAKE_CMD) modules

	$(hide) echo "Install kernel modules to system/lib/modules ..."
	$(hide) $(kernel_MAKE_CMD) INSTALL_MOD_PATH=$(DJI_SOURCE_TOP)/$(TARGET_OUT) modules_install
```


## 本地切clang编译wa234失败问题
报错：`clang-14:error: assembler commandfailed withexit code 1(use -v tosee invocation)`

把Makefile中这段话注释掉就好了，默认会使能`-no-integrated-as`，其含义是不使用集成的汇编器。注释掉后就是使用集成的汇编器，就不用自己搞一个as的bin了
```
# ifneq ($(LLVM_IAS),1)
# CLANG_FLAGS	+= -no-integrated-as
# endif
```















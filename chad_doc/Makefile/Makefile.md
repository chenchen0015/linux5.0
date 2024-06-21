https://www.liaoxuefeng.com/wiki/1599771213037600/1599771942846497

## Makefile基础
1、介绍
make虽然最初是针对C语言开发，但它实际上并不限定C语言，而是可以应用到任意项目，甚至不是编程语言。此外，make主要用于Unix/Linux环境的自动化开发，掌握Makefile的写法，可以更好地在Linux环境下做开发，也可以为后续开发Linux内核做好准备。

2、基础语法
```Makefile
# 目标文件: 依赖文件1 依赖文件2
m.txt: a.txt b.txt
	cat a.txt b.txt > m.txt
```
* 使用make运行
    * 只调用make，会在Makefile找第一个目标生成
    * make m.txt，会找m.txt这个目标去生成
* 再次运行make，make检测到x.txt已经是最新版本，无需再次执行，因为x.txt的创建时间晚于它依赖的m.txt和c.txt的最后修改时间

3、伪目标
```Makefile
clean:
	rm -f m.txt
	rm -f x.txt
```
运行make clean执行清除

* 如果目录下有一个文件名字叫做clean，那么执行make clean的时候会报错"make: 'clean' is up to date."
    * 为什么会这样子呢？我猜测逻辑是这样的：
    * 执行`make file`的时候，make会先检查file的创建日期，然后找到file依赖的文件 以及他们的创建日期，最后比较两者的时间先后
    * 对于此case，clean没有依赖其他文件，然后clean这个目标又存在，所以make认为clean已经是最新的了
    * 本质上是因为在执行命令之前会有一个版本比较的逻辑
* 所以需要告诉make，clean是一个伪目标，你编译的时候要去Makefile中找
    *   ```Makefile
        .PHONY: clean
        clean:
            rm -f m.txt
            rm -f x.txt
        ```
    * 此时，clean就不被视为一个文件，而是伪目标（Phony Target）。
    * 大型项目通常会提供clean、install这些约定俗成的伪目标名称，方便用户快速执行特定任务。
    * 一般来说，并不需要用.PHONY标识clean等约定俗成的伪目标名称，除非有人故意搞破坏，手动创建名字叫clean的文件。


4、执行多条命令
一个规则可以有多条命令，例如：
```Makefile
cd:
	pwd
	cd ..
	pwd
```

注意：执行多条命令时，每一条命令都有自己的shell环境，所以用`cd ../`并不能改变路径
解决方法
* 把多条命令以;分隔，写到一行
```Makefile
cd_ok:
	pwd; cd ..; pwd;

# 可以使用\把一行语句拆成多行，便于浏览：
cd_ok:
	pwd; \
	cd ..; \
	pwd
```

* 另一种执行多条命令的语法是用&&，它的好处是当某条命令失败时，后续命令不会继续执行
```Makefile
cd_ok:
	cd .. && pwd
```

5、控制打印
在语句前面加上`@`

6、控制错误
在语句前面加上`-`


## 使用变量
1、定义变量
变量定义用`变量名 = 值`或者`变量名 := 值`，通常变量名全大写。引用变量用`$(变量名)`，非常简单。

2、使用函数
```Makefile
# $(wildcard *.c) 列出当前目录下的所有 .c 文件: hello.c main.c
# 用函数 patsubst 进行模式替换得到: hello.o main.o
OBJS = $(patsubst %.c,%.o,$(wildcard *.c))
TARGET = world.out

$(TARGET): $(OBJS)
	cc -o $(TARGET) $(OBJS)

clean:
	rm -f *.o $(TARGET)
```

3、内置变量
```Makefile
$(TARGET): $(OBJS)
	$(CC) -o $(TARGET) $(OBJS)
```
没有定义变量CC也可以引用它，因为它是make的内置变量（Builtin Variables），表示C编译器的名字，默认值是cc

4、自动变量
```Makefile
world.out: hello.o main.o
	@echo '$$@ = $@' # 变量 $@ 表示target
	@echo '$$< = $<' # 变量 $< 表示第一个依赖项
	@echo '$$^ = $^' # 变量 $^ 表示所有依赖项
	cc -o $@ $^
```

## 使用模式规则
```Makefile
# 模式匹配规则：当make需要目标 xyz.o 时，自动生成一条 xyz.o: xyz.c 规则:
%.o: %.c
	@echo 'compiling $<...'
	cc -c -o $@ $<
```


## 自动生成依赖
背景：有个问题，按照上一节模式规则搞出来的Makefile，xyz.o只依赖xyz.c，但是xyz.c可能还依赖了一堆头文件。在修改头文件的时候，必须也要让xyz.o重新编译。

我们利用gcc -MM识别出每个.c都依赖哪些.h
```shell
$ cc -MM main.c
main.o: main.c hello.h
```
上述输出告诉我们，编译main.o依赖main.c和hello.h两个文件。

因此，我们可以利用GCC的这个功能，对每个.c文件都生成一个依赖项，通常我们把它保存到.d文件中，再用`include`引入到Makefile，就相当于自动化完成了每个.c文件的精准依赖。

我们改写上一节的Makefile如下：
```Makefile
# 列出所有 .c 文件:
SRCS = $(wildcard *.c)

# 根据SRCS生成 .o 文件列表:
OBJS = $(SRCS:.c=.o)

# 根据SRCS生成 .d 文件列表:
DEPS = $(SRCS:.c=.d)

TARGET = world.out

# 默认目标:
$(TARGET): $(OBJS)
	$(CC) -o $@ $^

# xyz.d 的规则由 xyz.c 生成:
%.d: %.c
	rm -f $@; \
	$(CC) -MM $< >$@.tmp; \
	sed 's,\($*\)\.o[ :]*,\1.o $@ : ,g' < $@.tmp > $@; \
	rm -f $@.tmp

# 模式规则:
%.o: %.c
	$(CC) -c -o $@ $<

clean:
	rm -rf *.o *.d $(TARGET)

# 引入所有 .d 文件:
include $(DEPS)
```





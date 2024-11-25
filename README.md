# Utility——find

这个项目实现了`find`命令的一些功能：

---

- 编译二进制文件：

    在终端中输入`make`来编译生成二进制文件

- 使用方法：
    
    输入`./myfind [option] [folders] [expression]`来使用程序

    - 选项：

        - `-d`：myfind会在处理目录的内容之前先处理该目录本身。这个选项遵循`BSD`系列的`find`行为，而不是`GNU`的`find`，在`GNU find`中，`-d`是一个表达式，而不是选项。默认情况下，myfind按照前序遍历目录（先处理目录本身，再处理目录的内容）

        - `-H`：myfind不会跟随符号链接，除非在命令行中明确指定

        - `-L`：myfind会跟随符号链接

        - `-P`：myfind永远不跟随符号链接，这是默认行为

- 清理`make`创建的文件：

    在终端中输入`make clean`来清理所有由`make`创建的文件

---

- 错误处理：
    
    - 如果在解析命令行时发生错误，程序会在标准错误（stderr）输出错误信息并退出，返回值为1

    - 如果在解析文件时发生错误，程序会继续执行，并在结束时返回1

    - 在其他情况下，函数返回 1

---

- 示例：

    ```shell
    # 编译
    make
    
    # 执行
    ./myfind
    
    # 执行带选项
    ./myfind -d folder1
    
    # 执行多个目录
    ./myfind -H . folder1 folder2
    
    # 执行多个选项和目录
    ./myfind -d -P -L folder1 folder2 folder3
    
    # 执行表达式
    ./myfind . -name '*.c*'    
    ./myfind include/ src/ 1>myfind.txt
    ./myfind -name '*.h' -exec cat {} \;
    # 清理
    make clean
    ```
# 设置编译器
CC = gcc

# 设置编译选项：
# 开启警告；
# 强制遵循ISO C标准（标准C的严格规则）；
# 指定使用C99标准；
# _DEFAULT_SOURCE 使得程序可以使用较新的 glibc 提供的功能；
# 添加额外的头文件搜索路径./include
CFLAGS = -Wall -pedantic -std=c99 -D_DEFAULT_SOURCE -I./include

# 目录配置
SRC_DIR = src
LIB_DIR = $(SRC_DIR)/lib
INCLUDE_DIR = include

# 库文件的源代码和生成的目标文件
LIB_SRC = $(LIB_DIR)/lib_str.c $(LIB_DIR)/lib_util.c
LIB_OBJ = $(LIB_SRC:.c=.o)
MYFIND_SRC = $(SRC_DIR)/myfind.c
MYFIND_OBJ = $(MYFIND_SRC:.c=.o)

# 输出的目标文件
TARGET = myfind

# 默认规则：生成目标可执行文件
all: $(TARGET)

# 链接目标可执行文件
$(TARGET): $(LIB_OBJ) $(MYFIND_OBJ)
	$(CC) $(CFLAGS) -o $@ $^

# 编译库文件
$(LIB_DIR)/%.o: $(LIB_DIR)/%.c $(INCLUDE_DIR)/lib/%.h
	$(CC) $(CFLAGS) -c -o $@ $<

# 编译主文件 myfind
$(SRC_DIR)/%.o: $(SRC_DIR)/%.c $(INCLUDE_DIR)/myfind.h
	$(CC) $(CFLAGS) -c -o $@ $<

# 清理生成的文件
clean:
	rm -f $(LIB_OBJ) $(MYFIND_OBJ) $(TARGET)

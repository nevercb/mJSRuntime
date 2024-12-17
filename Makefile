# 编译器
CC = gcc

# 包含路径（头文件目录）
CFLAGS = -Iquickjs -Iinclude

# 链接选项（静态库路径 + 所需的动态库）
LDFLAGS = quickjs/libquickjs.a -lm -ldl

# 输出的目标文件名
TARGET = runtime

# 源文件列表（在 src/ 目录中）
SRC = src/main.c \
      src/require.c \
      src/module_cache.c \
      src/console.c \
      src/event_loop.c

# 默认目标
all: $(TARGET)

# 链接生成可执行文件
$(TARGET): $(SRC)
	$(CC) $(SRC) $(CFLAGS) $(LDFLAGS) -o $(TARGET)

# 运行测试脚本
run: $(TARGET)
	./$(TARGET) test/test.js

# 清理生成的文件
clean:
	rm -f $(TARGET)
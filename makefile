cd ~/my_cpp_project
find . -type f \( -name "*.cpp" -o -name "*.h" \)


# 1. 编译器
CXX = g++

# 2. 编译选项（你可以添加 -g 进行调试，或 -O2 进行优化）
CXXFLAGS = -std=c++17 -Wall -Wextra

# 3. 包含头文件路径（如果有 .h 或第三方库的 include 路径）
INCLUDE_FLAGS = -I./include

# 4. 源代码文件（你需要修改这一行，把你的 .cpp 文件列出来）
SRCS = main.cpp foo.cpp bar.cpp

# 5. 对应的目标文件（自动把 .cpp 变成 .o）
OBJS = $(SRCS:.cpp=.o)

# 6. 最终生成的可执行文件名
TARGET = myprogram

# 7. 默认目标
all: $(TARGET)

# 8. 链接目标文件生成最终程序
$(TARGET): $(OBJS)
	$(CXX) $(CXXFLAGS) $(INCLUDE_FLAGS) $(OBJS) -o $@

# 9. 编译 .cpp → .o
%.o: %.cpp
	$(CXX) $(CXXFLAGS) $(INCLUDE_FLAGS) -c $< -o $@

# 10. 清理命令
clean:
	rm -f $(OBJS) $(TARGET)

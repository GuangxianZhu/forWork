## 下面给你一个「较大」C++ 项目目录示例 + 一个在项目根目录写的通用 Makefile（推荐）。它会把 ./sub/UnitTest/zhutest/*.cpp 编译成一个可执行文件，并且自动把项目里各处的 include 目录加到 -I 里，兼容复杂头文件组织。
your_project/
├─ Makefile                 # 根 Makefile（下面给出）
├─ include/                 # 公共头
│   └─ yourproj/...
├─ src/                     # 业务源码
│   ├─ core/...
│   └─ util/...
├─ modules/
│   ├─ net/
│   │  ├─ include/...
│   │  └─ src/...
│   └─ storage/
│      ├─ include/...
│      └─ src/...
├─ third_party/
│   ├─ libA/include/...
│   └─ libB/include/...
└─ sub/
   └─ UnitTest/
      └─ zhutest/
         └─ test_xxx.cpp   # 你要编译的单测源文件（也可以有多个 *.cpp）

根 Makefile（放在项目根目录）
# ================= [Project config] =================
PROJECT_NAME := zhutest
CXX          := g++
CXXSTD       := -std=c++17
WARNINGS     := -Wall -Wextra -Wpedantic
OPT          := -O2
# 自动生成依赖：.d 文件，避免手工维护头文件依赖
DEPFLAGS     := -MMD -MP

# 你可能需要的宏
DEFINES      :=

# —— 自动收集所有可能的 include 目录（递归找名为 include 的目录）——
# 如果你的环境不喜欢 $(shell find ...)，也可手工列出。
INCLUDE_DIRS := include \
                $(shell find modules -type d -name include 2>/dev/null) \
                $(shell find third_party -type d -name include 2>/dev/null)
CPPFLAGS     := $(addprefix -I,$(INCLUDE_DIRS)) $(addprefix -D,$(DEFINES))

CXXFLAGS     := $(CXXSTD) $(WARNINGS) $(OPT) $(DEPFLAGS)

# 库（如果需要）
LDFLAGS      :=
LDLIBS       :=

# ================= [Sources / Outputs] =================
# 只构建你的单测目录下的 cpp
TEST_SRCS    := $(wildcard sub/UnitTest/zhutest/*.cpp)

# 目标可执行文件
BIN_DIR      := bin
TARGET       := $(BIN_DIR)/$(PROJECT_NAME)

# 对象文件与依赖文件放到 build/ 下，保持目录层级
OBJ_DIR      := build
OBJS         := $(patsubst %.cpp,$(OBJ_DIR)/%.o,$(TEST_SRCS))
DEPS         := $(OBJS:.o=.d)

# ================= [Rules] =================
.PHONY: all run clean tidy includes

all: $(TARGET)

# 链接
$(TARGET): $(OBJS) | $(BIN_DIR)
	$(CXX) $(CXXFLAGS) $^ -o $@ $(LDFLAGS) $(LDLIBS)
	@echo ">>> built $@"

# 编译：把 sub/UnitTest/zhutest/*.cpp 编到 build/sub/UnitTest/zhutest/*.o
$(OBJ_DIR)/%.o: %.cpp | prep_dirs
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) $(CPPFLAGS) -c $< -o $@

# 目录预创建
prep_dirs: | $(BIN_DIR) $(OBJ_DIR)
$(BIN_DIR):
	@mkdir -p $@
$(OBJ_DIR):
	@mkdir -p $@

# 便捷：运行单测
run: $(TARGET)
	./$(TARGET)

# 清理
clean:
	@rm -rf $(OBJ_DIR) $(BIN_DIR)

# 显示最终 -I 列表（调试用）
includes:
	@echo $(CPPFLAGS)

# 自动包含依赖文件
-include $(DEPS)

怎么用
把上面的 Makefile 放在项目根目录。
把你的测试源文件放在 sub/UnitTest/zhutest/ 下（可一个或多个 *.cpp）。
在项目根执行：

make            # 编译
make run        # 运行 bin/zhutest
make clean      # 清理

如果你更想在 ./sub/UnitTest/zhutest 目录单独写一个小 Makefile
这种方式通常需要从子目录向上添加包含路径。最简模板如下（放在 sub/UnitTest/zhutest/Makefile）：
CXX := g++
CXXFLAGS := -std=c++17 -O2 -Wall -Wextra -MMD -MP

# 项目根（相对当前目录）
ROOT := $(abspath ../..)
# 手工列出或用 find 自动找 include 目录
INCLUDES := -I$(ROOT)/include \
            $(shell cd $(ROOT) && find modules -type d -name include -printf "-I$(ROOT)/%p ")

SRCS := $(wildcard *.cpp)
OBJS := $(SRCS:.cpp=.o)
DEPS := $(OBJS:.o=.d)
TARGET := zhutest

all: $(TARGET)
$(TARGET): $(OBJS)
	$(CXX) $(CXXFLAGS) $^ -o $@

%.o: %.cpp
	$(CXX) $(CXXFLAGS) $(INCLUDES) -c $< -o $@

clean:
	rm -f $(OBJS) $(DEPS) $(TARGET)

-include $(DEPS)


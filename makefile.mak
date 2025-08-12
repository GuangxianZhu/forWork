所有 .cpp（或 .c）里有实现的代码，如果它最终会被你的程序用到，都必须在编译时参与生成目标文件并参与链接。
zhumain.cpp
  |
  --> 调用 RS485.cpp 里的函数
         |
         --> 调用 CRC.cpp 里的函数
即使 zhumain.cpp 根本没 #include "CRC.hpp"，只要 RS485.cpp 调用了 CRC.cpp 里的实现，那么：

CRC.cpp 必须加到 SRCS

否则链接阶段就会报 undefined reference

Makefile 做法
	在 SRCS 里包含：
	当前测试用例的 .cpp
	它直接 #include 的 .hpp 对应的 .cpp
	这些 .cpp 再调用的别的 .hpp 对应的 .cpp
	直到调用链结束
	如果 .cpp 太多，可以用自动扫描的方法，把整个 src/、driver/ 下的 .cpp 都加进去，让链接器自己扔掉没用到的目标文件（静态库或可执行文件链接时会丢掉未引用的部分）。

在 sub/UnitTest/zhutest/ 下放一个 自动扫描版 Makefile
# ===== 基本配置 =====
CXX      := g++
CXXFLAGS := -std=c++17 -O2 -Wall -Wextra -MMD -MP

# 项目根（从 zhutest 向上两级）
ROOT := $(abspath ../..)

# 需要扫描源码/头文件的根
CODE_ROOTS := $(ROOT)/src $(ROOT)/driver $(ROOT)/drivers

# 递归收集 include 目录（含根本身）
INCLUDE_DIRS := $(CODE_ROOTS) $(shell find $(CODE_ROOTS) -type d 2>/dev/null)
CPPFLAGS     := $(addprefix -I,$(INCLUDE_DIRS))

# ===== 源文件收集 =====
# 当前目录下的测试源码
SRCS_LOCAL := $(wildcard *.cpp)

# 上层各目录递归收集 .cpp（不存在时 find 会静默）
SRCS_EXT := \
  $(shell [ -d "$(ROOT)/src" ]     && find "$(ROOT)/src"     -type f -name '*.cpp' 2>/dev/null) \
  $(shell [ -d "$(ROOT)/driver" ]  && find "$(ROOT)/driver"  -type f -name '*.cpp' 2>/dev/null) \
  $(shell [ -d "$(ROOT)/drivers" ] && find "$(ROOT)/drivers" -type f -name '*.cpp' 2>/dev/null)

# 合并所有源文件（带绝对/相对路径）
SRCS_ALL := $(SRCS_LOCAL) $(SRCS_EXT)

# 将“外部源文件”的前缀 $(ROOT)/ 去掉，做一个相对路径副本，用来映射到 build/ 目录
REL_SRCS := $(filter-out $(ROOT)/%,$(SRCS_ALL)) \
            $(patsubst $(ROOT)/%,%,$(filter $(ROOT)/%,$(SRCS_ALL)))

# 目标、中间文件放这里（保持目录层级，避免同名冲突）
BUILD  := build
BIN    := bin
TARGET := $(BIN)/zhutest
OBJS   := $(patsubst %.cpp,$(BUILD)/%.o,$(REL_SRCS))
DEPS   := $(OBJS:.o=.d)

# 可选库
LDFLAGS :=
LDLIBS  :=

# ===== 规则 =====
.PHONY: all run clean includes

all: $(TARGET)

# 链接
$(TARGET): $(OBJS) | $(BIN)
	$(CXX) $(CXXFLAGS) $(CPPFLAGS) $^ -o $@ $(LDFLAGS) $(LDLIBS)
	@echo ">>> built $@"

# --- 编译规则（两条）：兼容“外部源”(在 ROOT 下) 和 “本地源”(当前目录) ---

# 外部源：build/xxx.o 由 $(ROOT)/xxx.cpp 生成
$(BUILD)/%.o: $(ROOT)/%.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) $(CPPFLAGS) -c $< -o $@

# 本地源：build/xxx.o 由 ./xxx.cpp 生成
$(BUILD)/%.o: %.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) $(CPPFLAGS) -c $< -o $@

$(BIN):
	@mkdir -p $@

# 调试：打印所有 -I 路径和收集到的源
includes:
	@echo INCLUDES: $(CPPFLAGS)
	@echo SRCS_ALL: $(SRCS_ALL)

run: $(TARGET)
	./$(TARGET)

clean:
	@rm -rf $(BUILD) $(BIN)

# 头文件依赖
-include $(DEPS)

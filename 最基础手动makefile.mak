# ===== 基本配置（按需改） =====
CXX      := g++
CXXFLAGS := -std=c++17 -O2 -Wall -Wextra -MMD -MP
LDFLAGS  :=
LDLIBS   :=              # 例: -lpthread -lfmt

# 项目根：从 zhutest 向上两级
ROOT := $(abspath ../..)

# ===== 头文件搜索路径（手动列）=====
# 例：如果你想在代码里写 #include "RS485.hpp" 或 "drivers/RS485.hpp"
CPPFLAGS := \
  -I$(ROOT)/include \
  -I$(ROOT)/src \
  -I$(ROOT)/driver \
  -I$(ROOT)/drivers

# ===== 源文件（手动列）=====
# 当前目录的测试源
SRCS := \
  zhumain.cpp \
  zhuHelper.cpp

# 上层/外部实现（根据需要添加/删除）
SRCS += \
  $(ROOT)/drivers/RS485.cpp \
  $(ROOT)/src/crc/CRC.cpp
# 再加：$(ROOT)/src/xxx/yyy.cpp ... （不需要的就别加）

# ===== 构建目录与产物 =====
BUILD  := build
BIN    := bin
TARGET := $(BIN)/zhutest

# ---- 处理路径：把 $(ROOT)/ 开头的路径转成相对，为了把 .o 放到 build/ 下保持层级 ----
REL_SRCS := $(patsubst $(ROOT)/%,%,$(SRCS))
OBJS     := $(patsubst %.cpp,$(BUILD)/%.o,$(REL_SRCS))
DEPS     := $(OBJS:.o=.d)

.PHONY: all clean run includes

all: $(TARGET)

# 链接
$(TARGET): $(OBJS) | $(BIN)
	$(CXX) $(CXXFLAGS) $(CPPFLAGS) $^ -o $@ $(LDFLAGS) $(LDLIBS)
	@echo ">>> built $@"

# --- 编译规则（两条，分别处理本地源和 $(ROOT)/ 下的源） ---

# 本地源：build/foo.o <- ./foo.cpp
$(BUILD)/%.o: %.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) $(CPPFLAGS) -c $< -o $@

# 上层源：build/path/to/foo.o <- $(ROOT)/path/to/foo.cpp
$(BUILD)/%.o: $(ROOT)/%.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) $(CPPFLAGS) -c $< -o $@

# 目录
$(BIN):
	@mkdir -p $@

# 便捷
run: $(TARGET)
	./$(TARGET)

includes:
	@echo INCLUDES: $(CPPFLAGS)
	@echo SRCS:     $(SRCS)

clean:
	@rm -rf $(BUILD) $(BIN)

# 头文件依赖自动跟踪
-include $(DEPS)

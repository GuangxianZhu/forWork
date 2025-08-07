#include <string>
#include <vector>
#include <iostream>
#include <cstdint>
#include <cstdio>

// ===== 1) 基础工具：严格解析 "A3 07 FF" =====
uint8_t hexval(char c) {
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'A' && c <= 'F') return c - 'A' + 10;
    return 255; // 非法
}

bool parse_hex_bytes_strict(const std::string& s, std::vector<uint8_t>& out) {
    out.clear();
    size_t i = 0;
    while (i < s.size()) {
        if (i + 1 >= s.size()) return false; // 少半个字节
        char c1 = s[i], c2 = s[i+1];
        uint8_t hi = hexval(c1), lo = hexval(c2);
        if (hi == 255 || lo == 255) return false;

        out.push_back((hi << 4) | lo);
        i += 2;

        if (i < s.size()) {         // 必须是单个空格分隔
            if (s[i] != ' ') return false;
            ++i;
        }
    }
    return true;
}

// ===== 2) 模板展开：-1 表示占位符，需要用户字节来填 =====
// 例：模板 {0xAA, 0x55, -1, -1, 0xE0, 0x01, -1, 0x0D}
//     用户输入 "A3 07 FF" -> 得到 {0xAA,0x55,0xA3,0x07,0xE0,0x01,0xFF,0x0D}
bool expand_template_with_input(const std::vector<int>& tpl,
                                const std::vector<uint8_t>& user,
                                std::vector<uint8_t>& out) {
    out.clear();
    size_t need = 0;
    for (size_t i = 0; i < tpl.size(); ++i) {
        if (tpl[i] == -1) ++need;
    }
    if (need != user.size()) return false; // 用户字节数必须与占位符数量一致

    size_t u = 0;
    for (size_t i = 0; i < tpl.size(); ++i) {
        if (tpl[i] == -1) {
            out.push_back(user[u++]);      // 用用户输入替换占位符
        } else {
            // 固定 opcode/常量
            out.push_back(static_cast<uint8_t>(tpl[i] & 0xFF));
        }
    }
    return true;
}

int main() {
    // ===== 3) 定义你的消息模板（中间夹固定 opcode）=====
    // -1 是占位符；其他是固定十六进制常量
    std::vector<int> msg_tpl = {
        0xAA, 0x55,        // 固定头
        -1, -1,            // 用户填两个字节（比如地址、高字节/低字节等）
        0xE0, 0x01,        // 固定 opcode 在中间
        -1,                // 用户再填一个字节（比如长度/命令参数）
        0x0D               // 固定结束符
    };

    // ===== 4) 读取用户输入并解析 =====
    std::cout << "请输入用户字节（大写，空格分隔，例如：A3 07 FF）：";
    std::string line;
    std::getline(std::cin, line);

    std::vector<uint8_t> user_bytes;
    if (!parse_hex_bytes_strict(line, user_bytes)) {
        std::cerr << "输入格式非法：必须是大写十六进制，字节间单空格分隔，如 A3 07 FF\n";
        return 1;
    }

    // ===== 5) 用用户字节填充模板 =====
    std::vector<uint8_t> final_msg;
    if (!expand_template_with_input(msg_tpl, user_bytes, final_msg)) {
        std::cerr << "用户字节数量与占位符数量不匹配！\n";
        return 1;
    }

    // ===== 6) 打印结果（用于确认；实际可直接 write(fd, ...) 发送）=====
    std::cout << "最终报文：";
    for (size_t i = 0; i < final_msg.size(); ++i) {
        std::printf("%02X ", final_msg[i]);
    }
    std::cout << "\n";
    return 0;
}

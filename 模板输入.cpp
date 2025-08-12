#include <string>
#include <vector>
#include <iostream>
#include <cstdint>
#include <cstdio>

// ---- 1) 更直观：半字节转换（仅允许 0-9 A-F，大写）----
inline int hex_nibble(char c) {
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'A' && c <= 'F') return c - 'A' + 10;
    return -1; // 非法
}

// 严格解析 "A3 07 FF"：两位十六进制，组间必须单空格；不允许空输入/多空格/小写
bool parse_hex_bytes_strict(const std::string& s, std::vector<uint8_t>& out, size_t* err_pos = nullptr) {
    out.clear();
    if (s.size() < 2) { if (err_pos) *err_pos = 0; return false; }

    size_t i = 0;
    while (true) {
        // 读两个十六进制字符
        if (i + 1 >= s.size()) { if (err_pos) *err_pos = i; return false; }
        int hi = hex_nibble(s[i]);
        int lo = hex_nibble(s[i + 1]);
        if (hi < 0 || lo < 0)   { if (err_pos) *err_pos = (hi < 0 ? i : i+1); return false; }

        out.push_back(static_cast<uint8_t>((hi << 4) | lo));
        i += 2;

        // 如果到达结尾，成功
        if (i == s.size()) return true;

        // 非结尾处，必须紧跟一个空格
        if (s[i] != ' ') { if (err_pos) *err_pos = i; return false; }
        ++i;

        // 空格后必须还有两个字符
        if (i + 1 >= s.size()) { if (err_pos) *err_pos = i; return false; }
    }
}

// ---- 2) 模板展开：-1 为占位，其余必须在 0..255 ----
bool expand_template_with_input(const std::vector<int>& tpl,
                                const std::vector<uint8_t>& user,
                                std::vector<uint8_t>& out,
                                size_t* err_index = nullptr) {
    out.clear();

    size_t need = 0;
    for (int v : tpl) if (v == -1) ++need;
    if (need != user.size()) { if (err_index) *err_index = need; return false; }

    size_t u = 0;
    for (size_t i = 0; i < tpl.size(); ++i) {
        int v = tpl[i];
        if (v == -1) {
            out.push_back(user[u++]);
        } else {
            if (v < 0 || v > 255) { if (err_index) *err_index = i; return false; }
            out.push_back(static_cast<uint8_t>(v));
        }
    }
    return true;
}

// ---- 3) 演示主程序 ----
int main() {
    std::vector<int> msg_tpl = {
        0xAA, 0x55,
        -1, -1,
        0xE0, 0x01,
        -1,
        0x0D
    };

    std::cout << "请输入用户字节（大写，空格分隔，例如：A3 07 FF）：";
    std::string line;
    std::getline(std::cin, line);

    std::vector<uint8_t> user_bytes;
    size_t errp = 0;
    if (!parse_hex_bytes_strict(line, user_bytes, &errp)) {
        std::cerr << "输入格式非法（位置 " << errp << "），必须是大写十六进制并以单空格分隔，如 A3 07 FF\n";
        return 1;
    }

    std::vector<uint8_t> final_msg;
    size_t erri = 0;
    if (!expand_template_with_input(msg_tpl, user_bytes, final_msg, &erri)) {
        std::cerr << "模板展开失败：占位数与输入字节不匹配或模板值越界（索引 " << erri << "）。\n";
        return 1;
    }

    std::cout << "最终报文：";
    for (uint8_t b : final_msg) std::printf("%02X ", b);
    std::cout << "\n";
    return 0;
}

#include <string>
#include <vector>
#include <cctype>
#include <iostream>
如果用户输入格式很规整，比如 A3 07 FF 这种：
	•	字节用空格分隔
	•	必须是大写字母（A-F）或者数字（0-9）
	•	没有 0x 前缀、逗号、短横线等其他符号

那解析函数就可以写得非常简单，而且执行效率也高。

bool parse_hex_bytes_strict(const std::string& s, std::vector<uint8_t>& out) {
    out.clear();
    size_t i = 0;
    while (i < s.size()) {
        // 读取高 4 位
        if (i + 1 >= s.size()) return false; // 少半个字节
        char c1 = s[i];
        char c2 = s[i+1];

        // 检查是否是合法的大写十六进制字符
        if (!((c1 >= '0' && c1 <= '9') || (c1 >= 'A' && c1 <= 'F'))) return false;
        if (!((c2 >= '0' && c2 <= '9') || (c2 >= 'A' && c2 <= 'F'))) return false;

        auto hexval = [](char c) -> uint8_t {
            return (c >= '0' && c <= '9') ? c - '0' : c - 'A' + 10;
        };

        uint8_t byte = (hexval(c1) << 4) | hexval(c2);
        out.push_back(byte);

        i += 2;

        // 如果后面还有字符，必须是空格
        if (i < s.size()) {
            if (s[i] != ' ') return false;
            ++i; // 跳过空格
        }
    }
    return true;
}

int main() {
    std::string input;
    std::vector<uint8_t> bytes;

    std::cout << "Enter hex bytes (e.g. A3 07 FF): ";
    std::getline(std::cin, input);

    if (!parse_hex_bytes_strict(input, bytes)) {
        std::cerr << "Invalid input format.\n";
        return 1;
    }

    std::cout << "Parsed bytes: ";
    for (auto b : bytes) {
        std::cout << std::hex << std::uppercase
                  << (b >> 4 & 0xF) << (b & 0xF) << ' ';
    }
    std::cout << std::dec << "\n";
}

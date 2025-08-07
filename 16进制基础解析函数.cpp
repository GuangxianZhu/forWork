#include <string>
#include <vector>
#include <cctype>
#include <iostream>

#include <string>
#include <vector>
#include <iostream>
#include <cstdint> // uint8_t

// 把一个十六进制字符转成数值（只支持 '0'-'9' 和 'A'-'F'）
uint8_t hexval(char c) {
    if (c >= '0' && c <= '9') {
        return c - '0';        // '3' -> 3
    } else if (c >= 'A' && c <= 'F') {
        return c - 'A' + 10;   // 'A' -> 10
    } else {
        return 255;            // 非法字符标记
    }
}

// 解析严格格式："A3 07 FF"
bool parse_hex_bytes_strict(const std::string& s, std::vector<uint8_t>& out) {
    out.clear();
    size_t i = 0;

    while (i < s.size()) {
        // 检查是否有两个字符可读
        if (i + 1 >= s.size()) {
            return false; // 少半个字节
        }

        char c1 = s[i];
        char c2 = s[i + 1];

        uint8_t high = hexval(c1);
        uint8_t low  = hexval(c2);

        // 检查字符是否合法
        if (high == 255 || low == 255) {
            return false;
        }

        // 合成一个字节
        out.push_back((high << 4) | low);

        i += 2; // 跳过刚读的两个字符

        // 如果还有字符，必须是空格
        if (i < s.size()) {
            if (s[i] != ' ') return false;
            i++; // 跳过空格
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
        printf("%02X ", b);
    }
    std::cout << "\n";
}

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

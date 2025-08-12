#include <iostream>
#include <string>
#include <cctype> // isxdigit

bool isValidHexFormat(const std::string &input) {
    if (input.empty()) return false;

    size_t i = 0;
    bool firstGroup = true;

    while (i < input.size()) {
        // 检查两位十六进制字符
        if (i + 1 >= input.size()) return false; // 不够两位
        if (!std::isxdigit(input[i]) || !std::isxdigit(input[i + 1])) {
            return false; // 不是十六进制字符
        }
        i += 2;

        // 如果到末尾，直接结束
        if (i == input.size()) break;

        // 检查分隔符（必须是一个空格）
        if (input[i] != ' ') return false;
        i++; // 跳过空格

        // 循环继续检查下一组
        firstGroup = false;
    }

    return true;
}

std::string readStrictHexInput_NoRegex() {
    std::string input;
    while (true) {
        std::cout << "请输入16进制字符串（格式如 02 A3 4D）：";
        std::getline(std::cin, input);

        if (isValidHexFormat(input)) {
            return input;
        } else {
            std::cout << "格式错误！必须是两位十六进制数，空格分隔，例如 02 A3 4D。\n";
        }
    }
}

int main() {
    std::string hexString = readStrictHexInput_NoRegex();
    std::cout << "你输入的是：" << hexString << std::endl;
    return 0;
}

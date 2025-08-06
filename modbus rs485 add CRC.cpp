#include <iostream>
#include <vector>
#include <sstream>
#include <iomanip>
#include <fcntl.h>
#include <unistd.h>
#include <termios.h>
#include <stdint.h>

// ===== 串口配置函数 =====
int setup_serial(const char* port_name) {
    int fd = open(port_name, O_RDWR | O_NOCTTY | O_NDELAY);
    if (fd < 0) {
        perror("无法打开串口");
        return -1;
    }

    struct termios options;
    tcgetattr(fd, &options);
    cfsetispeed(&options, B9600);
    cfsetospeed(&options, B9600);

    options.c_cflag &= ~PARENB; // 无校验
    options.c_cflag &= ~CSTOPB; // 1 停止位
    options.c_cflag &= ~CSIZE;
    options.c_cflag |= CS8;     // 8 数据位
    options.c_cflag |= (CLOCAL | CREAD);

    options.c_lflag &= ~(ICANON | ECHO | ECHOE | ISIG);
    options.c_iflag &= ~(IXON | IXOFF | IXANY);
    options.c_oflag &= ~OPOST;

    tcsetattr(fd, TCSANOW, &options);
    return fd;
}

// ===== 解析十六进制输入 =====
std::vector<uint8_t> parse_hex_input(const std::string& input) {
    std::vector<uint8_t> data;
    std::stringstream ss(input);
    unsigned int byte;
    while (ss >> std::hex >> byte) {
        data.push_back(static_cast<uint8_t>(byte));
    }
    return data;
}

// ===== CRC16（Modbus RTU）计算 =====
uint16_t modbus_crc16(const uint8_t* data, size_t len) {
    uint16_t crc = 0xFFFF;
    for (size_t pos = 0; pos < len; ++pos) {
        crc ^= (uint16_t)data[pos];
        for (int i = 0; i < 8; i++) {
            if (crc & 0x0001)
                crc = (crc >> 1) ^ 0xA001;
            else
                crc >>= 1;
        }
    }
    return crc;
}

// ===== 打印数据 =====
void print_hex(const uint8_t* data, int len) {
    for (int i = 0; i < len; ++i) {
        printf("%02X ", data[i]);
    }
    std::cout << std::endl;
}

// ===== 主程序 =====
int main() {
    const char* port = "/dev/ttyUSB0"; // 修改为你的串口
    int fd = setup_serial(port);
    if (fd < 0) return 1;

    std::string input;
    std::cout << "请输入十六进制数据（不包含CRC，空格分隔，如：01 03 00 00 00 01）:" << std::endl;
    std::getline(std::cin, input);

    std::vector<uint8_t> tx_data = parse_hex_input(input);
    if (tx_data.empty()) {
        std::cerr << "输入为空或格式错误。" << std::endl;
        close(fd);
        return 1;
    }

    // 自动计算并追加 CRC16（低字节在前）
    uint16_t crc = modbus_crc16(tx_data.data(), tx_data.size());
    tx_data.push_back(crc & 0xFF);        // CRC低字节
    tx_data.push_back((crc >> 8) & 0xFF); // CRC高字节

    std::cout << "发送内容（含CRC）：" << std::endl;
    print_hex(tx_data.data(), tx_data.size());

    // 写入串口
    write(fd, tx_data.data(), tx_data.size());
    std::cout << "已发送。" << std::endl;

    // 接收响应
    usleep(100000); // 100ms
    uint8_t rx[256];
    int n = read(fd, rx, sizeof(rx));

    if (n > 0) {
        std::cout << "收到响应：" << n << " 字节：" << std::endl;
        print_hex(rx, n);
    } else {
        std::cout << "未收到响应。" << std::endl;
    }

    close(fd);
    return 0;
}

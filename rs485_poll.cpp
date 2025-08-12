工作流程
打开串口 → /dev/ttyUSB0（或你的 MOXA RealCOM 设备）
发送请求帧（poll_frame）
等待短暂时间（让设备响应）
读取接收缓冲
处理收到的数据（这里只是打印）
延时到下一轮

#include <iostream>
#include <vector>
#include <string>
#include <unistd.h>
#include <fcntl.h>
#include <cerrno>
#include <cstring>
#include <chrono>
#include <thread>

// 轮询发送的数据帧（示例）
static const uint8_t poll_frame[] = { 0x01, 0x03, 0x00, 0x00, 0x00, 0x01, 0x84, 0x0A }; 
// 这里用的是Modbus RTU读寄存器的例子；你可替换成厂家协议帧

bool rs485_polling(int fd, int interval_ms) {
    uint8_t buf[256];

    while (true) {
        // 1. 发送轮询帧
        ssize_t wlen = write(fd, poll_frame, sizeof(poll_frame));
        if (wlen < 0) {
            std::cerr << "写入失败: " << std::strerror(errno) << "\n";
            return false;
        }
        std::cout << "[TX] ";
        for (size_t i = 0; i < sizeof(poll_frame); ++i) {
            printf("%02X ", poll_frame[i]);
        }
        std::cout << "\n";

        // 2. 等待一小会再读（避免立即读到自己发的回环或空）
        std::this_thread::sleep_for(std::chrono::milliseconds(50));

        // 3. 读取回复（非阻塞可用 VTIME 控制超时；阻塞则会等到数据来）
        ssize_t rlen = read(fd, buf, sizeof(buf));
        if (rlen > 0) {
            std::cout << "[RX] ";
            for (ssize_t i = 0; i < rlen; ++i) {
                printf("%02X ", buf[i]);
            }
            std::cout << "\n";
        } else if (rlen == 0) {
            std::cout << "[RX] 超时无数据\n";
        } else {
            std::cerr << "读取失败: " << std::strerror(errno) << "\n";
            return false;
        }

        // 4. 等待到下一次轮询
        std::this_thread::sleep_for(std::chrono::milliseconds(interval_ms));
    }
    return true;
}

int main() {
    // 打开串口（假设已用 termios 配置好）
    int fd = open("/dev/ttyUSB0", O_RDWR | O_NOCTTY);
    if (fd < 0) {
        std::cerr << "打开串口失败: " << std::strerror(errno) << "\n";
        return 1;
    }

    // 直接轮询，每 1 秒发一次
    rs485_polling(fd, 1000);

    close(fd);
    return 0;
}

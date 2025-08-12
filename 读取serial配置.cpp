# 串口配置（key value；空格分隔；# 开头的行是注释）
device /dev/ttyUSB0

# 波特率可选：9600 19200 38400 57600 115200（需要更高速再加映射）
baud 115200

# 数据位：7 或 8
data_bits 8

# 校验位：N/E/O
parity N

# 停止位：1 或 2
stop_bits 1

# 流控：None / XONXOFF / RTSCTS
flow None

# 是否阻塞读取（阻塞=read直到读到>=read_min字节；非阻塞受timeout_ms影响）
blocking_read true

# VMIN（最小读字节数），仅阻塞读取有效；常用 1
read_min 1

# VTIME（以毫秒指定；会换算成termios的1=100ms）。仅在非阻塞读取时有效
timeout_ms 200


#include <iostream>
#include <fstream>
#include <string>
#include <termios.h>
#include <unistd.h>
#include <fcntl.h>
#include <cerrno>
#include <cstring>
#include <algorithm>

struct SerialConfig {
    std::string device = "/dev/ttyUSB0";
    int baud       = 115200; // 9600/19200/38400/57600/115200（可自行扩展）
    int data_bits  = 8;      // 7/8
    char parity    = 'N';    // N/E/O
    int stop_bits  = 1;      // 1/2
    std::string flow = "None"; // None/XONXOFF/RTSCTS
    bool blocking_read = true; // 阻塞或非阻塞
    int read_min  = 1;       // VMIN（阻塞下常用为1）
    int timeout_ms = 200;    // VTIME（仅非阻塞时生效，ms）
};

static speed_t to_speed_t(int baud) {
    switch (baud) {
        case 9600:   return B9600;
        case 19200:  return B19200;
        case 38400:  return B38400;
        case 57600:  return B57600;
        case 115200: return B115200;
        // 需要更高速：自己加 case 230400/460800/921600 并确保系统支持
        default:     return 0;
    }
}

static std::string upper(std::string s) {
    std::transform(s.begin(), s.end(), s.begin(),
                   [](unsigned char c){ return std::toupper(c);});
    return s;
}

// 读取最简单的“key value”格式；空行与以 # 开头的行跳过
bool load_csv_simple(const std::string& filename, SerialConfig& cfg) {
    std::ifstream fin(filename);
    if (!fin) {
        std::cerr << "无法打开CSV文件: " << filename << "\n";
        return false;
    }
    std::string key, value;
    while (fin >> key) {
        if (key.size() && key[0] == '#') { // 本行是注释：丢弃整行
            std::string dummy;
            std::getline(fin, dummy);
            continue;
        }
        if (!(fin >> value)) break;

        if (key == "device") cfg.device = value;
        else if (key == "baud") cfg.baud = std::stoi(value);
        else if (key == "data_bits") cfg.data_bits = std::stoi(value);
        else if (key == "parity") cfg.parity = upper(value)[0];
        else if (key == "stop_bits") cfg.stop_bits = std::stoi(value);
        else if (key == "flow") cfg.flow = upper(value);          // 统一转大写比较方便
        else if (key == "blocking_read") {
            std::string u = upper(value);
            cfg.blocking_read = (u=="TRUE"||u=="1"||u=="Y"||u=="YES");
        }
        else if (key == "read_min") cfg.read_min = std::stoi(value);
        else if (key == "timeout_ms") cfg.timeout_ms = std::stoi(value);
        else {
            std::cerr << "忽略未知键: " << key << "\n";
        }
    }
    return true;
}

bool apply_config(int fd, const SerialConfig& cfg) {
    termios tio{};
    if (tcgetattr(fd, &tio) != 0) {
        std::cerr << "tcgetattr失败: " << std::strerror(errno) << "\n";
        return false;
    }

    // 原始模式更适合二进制协议
    cfmakeraw(&tio);
    tio.c_cflag |= (CLOCAL | CREAD);

    // 波特率
    speed_t spd = to_speed_t(cfg.baud);
    if (spd == 0) {
        std::cerr << "不支持的波特率: " << cfg.baud << "\n";
        return false;
    }
    cfsetispeed(&tio, spd);
    cfsetospeed(&tio, spd);

    // 数据位
    tio.c_cflag &= ~CSIZE;
    if (cfg.data_bits == 8)      tio.c_cflag |= CS8;
    else if (cfg.data_bits == 7) tio.c_cflag |= CS7;
    else {
        std::cerr << "不支持的数据位: " << cfg.data_bits << "\n";
        return false;
    }

    // 校验
    tio.c_cflag &= ~(PARENB | PARODD);
    if (cfg.parity == 'E')             tio.c_cflag |= PARENB;           // 偶
    else if (cfg.parity == 'O')        tio.c_cflag |= (PARENB | PARODD); // 奇
    // N：都关即可

    // 停止位
    if (cfg.stop_bits == 2) tio.c_cflag |= CSTOPB;
    else                    tio.c_cflag &= ~CSTOPB;

    // 流控
    tio.c_iflag &= ~(IXON | IXOFF | IXANY);
#ifdef CRTSCTS
    tio.c_cflag &= ~CRTSCTS;
#endif
    if (cfg.flow == "XONXOFF") {
        tio.c_iflag |= (IXON | IXOFF);      // 软件流控
    } else if (cfg.flow == "RTSCTS") {
#ifdef CRTSCTS
        tio.c_cflag |= CRTSCTS;            // 硬件流控
#else
        std::cerr << "系统未定义硬件流控(CRTSCTS)，忽略。\n";
#endif
    } // None：已清空

    // 阻塞/非阻塞读取与超时（核心：VMIN/VTIME）
    if (cfg.blocking_read) {
        // 阻塞：read() 直到读到 >= VMIN 字节返回；VTIME 忽略
        int vmin = cfg.read_min;
        if (vmin < 1) vmin = 1;
        if (vmin > 255) vmin = 255;
        tio.c_cc[VMIN]  = static_cast<cc_t>(vmin);
        tio.c_cc[VTIME] = 0;
    } else {
        // 非阻塞：VMIN=0，VTIME=timeout/100ms（0~255）
        tio.c_cc[VMIN] = 0;
        int vtime = cfg.timeout_ms / 100; // 1=100ms
        if (vtime < 0) vtime = 0;
        if (vtime > 255) vtime = 255;
        tio.c_cc[VTIME] = static_cast<cc_t>(vtime);
    }

    if (tcsetattr(fd, TCSANOW, &tio) != 0) {
        std::cerr << "tcsetattr失败: " << std::strerror(errno) << "\n";
        return false;
    }
    return true;
}

int main(int argc, char** argv) {
    std::string path = "serial_config.csv";
    if (argc >= 2) path = argv[1];

    SerialConfig cfg;
    if (!load_csv_simple(path, cfg)) return 1;

    int fd = open(cfg.device.c_str(), O_RDWR | O_NOCTTY);
    if (fd < 0) {
        std::cerr << "打开设备失败: " << cfg.device << " : " << std::strerror(errno) << "\n";
        return 1;
    }

    if (!apply_config(fd, cfg)) {
        close(fd);
        return 1;
    }

    std::cout << "串口已配置完成："
              << " dev=" << cfg.device
              << " baud=" << cfg.baud
              << " data=" << cfg.data_bits
              << " parity=" << cfg.parity
              << " stop=" << cfg.stop_bits
              << " flow=" << cfg.flow
              << " blocking=" << (cfg.blocking_read?"Y":"N")
              << " read_min=" << cfg.read_min
              << " timeout_ms=" << cfg.timeout_ms
              << "\n";

    // 这里可以继续你的收发逻辑（read()/write()）
    // ...

    close(fd);
    return 0;
}

编译与运行
g++ -std=c++17 -O2 serial_from_csv.cpp -o serial_from_csv
./serial_from_csv                 # 读取默认 ./serial_config.csv
./serial_from_csv myconf.csv      # 指定其他配置文件


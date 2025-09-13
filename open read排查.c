// ******************************
// 检查open
// ******************************
int fd = ::open(dev, O_RDWR | O_NOCTTY | O_NONBLOCK);
if (fd < 0) {
    int e = errno;
    std::cerr << "open() failed: " << strerror(e)
                << " (errno=" << e << ")" << std::endl;

    switch (e) {
        case ENOENT: std::cerr << "Hint: 设备节点不存在，检查驱动/路径。\n"; break;
        case EACCES: std::cerr << "Hint: 权限不足，加入 dialout 组或 sudo 运行。\n"; break;
        case EBUSY:  std::cerr << "Hint: 设备被占用，检查 lsof/fuser。\n"; break;
        case ENODEV: std::cerr << "Hint: 设备无效或驱动未加载。\n"; break;
        default:     std::cerr << "Hint: 查看 dmesg 日志以获取更多信息。\n"; break;
    }
    return 1;
}

std::cout << "open() success, fd=" << fd << std::endl;

// ******************************
// 检查read
// ******************************
检查是否被占用
bash:
sudo lsof /dev/ttyr00
sudo fuser /dev/ttyr00

read() 返回 -1 的错误码分流
ssize_t n = read(fd, buf, sizeof(buf));
if (n == -1) {
    int e = errno; // 先保存
    std::cerr << "read error: " << strerror(e) << " (errno=" << e << ")\n";
    switch (e) {
        case EAGAIN: std::cerr << "Hint: 非阻塞且暂无数据，使用 poll()/select() 等待或改阻塞/设置 VMIN/VTIME。\n"; break;
        case EINTR:  std::cerr << "Hint: 被信号中断，直接重试或屏蔽相关信号。\n"; break;
        case EBADF:  std::cerr << "Hint: fd 非法或不可读，检查 open() 标志与返回值。\n"; break;
        case EACCES: std::cerr << "Hint: 权限不足，将用户加入 dialout 组或 sudo 运行。\n"; break;
        case EBUSY:  std::cerr << "Hint: 设备被占用，用 fuser/lsof 找到并关闭占用进程。\n"; break;
        case EIO:    std::cerr << "Hint: I/O 错误，检查线缆、MOXA 状态、dmesg 日志。\n"; break;
        default:     std::cerr << "Hint: 结合 dmesg、stty、驱动与 MOXA 配置进一步定位。\n";
    }
}

在程序里打印 errno，对应处理：
| errno                | 含义        | 处理方法                                            |
| -------------------- | --------- | ----------------------------------------------- |
| `EAGAIN/EWOULDBLOCK` | 非阻塞模式下无数据 | 用 `poll()/select()` 等待数据到来；或阻塞模式+VMIN/VTIME 控制。 |
| `EINTR`              | 被信号中断     | 直接重试；确认无 SIGALRM 等干扰。                           |
| `EBADF`              | `fd` 非法   | 检查 `open()` 返回值，确认 `O_RDWR` 成功。                 |
| `EACCES/EPERM`       | 权限不足      | 确认用户在 dialout 组。                                |
| `EBUSY`              | 设备忙       | 用 `lsof/fuser` 找占用进程。                           |
| `EIO`                | I/O 错误    | 见下文 3. 物理层与 MOXA 配置。                            |
| `EINVAL`             | 参数无效      | 检查 `buf!=NULL`、`size>0`。                        |
| `ENXIO/ENODEV`       | 设备不存在     | 节点被拔掉/驱动异常，重启驱动或确认设备状态。                         |
| `EFAULT`             | 无效指针      | 确认传入缓冲区合法。                                      |

调试步骤建议

确认写成功
在 write() 后检查返回值，确认确实发出了字节。

如果 write() 返回 -1 → 看 errno（同上表）。

如果返回值 < 期望长度 → 说明没全发完。


////////////////////////////////
class CMOXACommHandler
public
    ...
CMOXACommHandler::commHandler(int ttyr, char* sendBuf, char* recvBuf){
    scc2698Init(ttyr);
    scc2698write(sendBuf, sizeof(sendBuf));
    scc2698read(recvBuf, sizeof(recvBuf));
}

// 调用处
CMOXACommHandler moxa;
char sendBuf[256] = {0};
char recvBuf[256] = {0};
moxa.commHandler(8, sendBuf, recvBuf);
////////////////////////////////
////////////////////////////////
update:
int commHandler(int ttyr, 
                const char* sendBuf, size_t sendLen,
                char* recvBuf, size_t recvCap) {
    scc2698Init(ttyr);
    scc2698write(sendBuf, sendLen);
    scc2698read(recvBuf, recvCap);
}
// 调用处
CMOXACommHandler moxa;
char sendBuf[256] = {0};
char recvBuf[256] = {0};
int rn = moxa.commHandler(8, sendBuf, sizeof(sendBuf), recvBuf, sizeof(recvBuf));
////////////////////////////////
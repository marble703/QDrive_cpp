#include <atomic>
#include <csignal>
#include <fstream>
#include <iostream>
#include <string>
#include <thread>
#include <memory>

#include "serial/serial.hpp"

using namespace qdriver::io;

static std::atomic<bool> g_running{true};

void handle_sigint(int) {
    g_running = false;
}

struct Options {
    std::string port    = "/dev/ttyACM0";
    uint32_t baud       = 115200;
    std::string outfile = "serial_out.txt";
    std::size_t chunk   = 256; // 每次读取的字节数
};

void print_usage(const char* prog) {
    std::cout << "用法: " << prog << " -p <port> -b <baud> -o <outfile> [-s <chunk>]" << std::endl
              << "示例: " << prog << " -p /dev/ttyUSB0 -b 115200 -o data.txt -s 512" << std::endl;
}

bool parse_args(int argc, char** argv, Options& opt) {
    for (int i = 1; i < argc; ++i) {
        std::string a = argv[i];
        if ((a == "-h") || (a == "--help")) {
            print_usage(argv[0]);
            return false;
        }
        if ((a == "-p" || a == "--port") && i + 1 < argc) {
            opt.port = argv[++i];
            continue;
        }
        if ((a == "-b" || a == "--baud") && i + 1 < argc) {
            opt.baud = std::stoul(argv[++i]);
            continue;
        }
        if ((a == "-o" || a == "--out") && i + 1 < argc) {
            opt.outfile = argv[++i];
            continue;
        }
        if ((a == "-s" || a == "--size") && i + 1 < argc) {
            opt.chunk = std::stoul(argv[++i]);
            continue;
        }
        std::cerr << "未知参数: " << a << std::endl;
        print_usage(argv[0]);
        return false;
    }
    return true;
}

int main(int argc, char** argv) {
    std::signal(SIGINT, handle_sigint);

    Options opt;
    if (!parse_args(argc, argv, opt)) {
        return 1;
    }

    // 准备 IO 上下文
    auto io_ctx  = std::make_shared<IoContext>();
    IOContextPtrSelector io_sel(io_ctx);

    // 配置串口（当前实现只支持设置波特率/数据位/校验位/停止位）
    SerialPortConfig cfg;
    cfg.baud_rate = opt.baud;
    cfg.data_bits = 8;
    cfg.parity    = SerialPortBase::parity::none;
    cfg.stop_bits = SerialPortBase::stop_bits::one;

    std::unique_ptr<Serial> ser;
    try {
        ser = std::make_unique<Serial>(std::move(io_sel), opt.port, cfg, opt.port);
    } catch (const std::exception& e) {
        std::cerr << "打开串口异常: " << e.what() << std::endl;
        return 2;
    }

    if (!ser->isOpen()) {
        std::cerr << "打开串口失败: port=" << opt.port << std::endl;
        return 2;
    }

    std::ofstream ofs(opt.outfile, std::ios::binary | std::ios::out | std::ios::app);
    if (!ofs.is_open()) {
        std::cerr << "打开输出文件失败: " << opt.outfile << std::endl;
        return 3;
    }

    std::cout << "已连接串口: " << opt.port << ", 波特率: " << opt.baud
              << ", 输出文件: " << opt.outfile << std::endl;
    std::cout << "提示: Ctrl+C 结束. 直接输入回车发送到串口." << std::endl;

    // 读线程：从串口读取数据并写入文件/标准输出
    std::thread reader([&]() {
        std::string buf;
        buf.resize(opt.chunk);

        while (g_running.load()) {
            auto bytesRead = std::make_shared<std::size_t>(0);
            bool ok = ser->read(buf, opt.chunk, bytesRead);
            if (!ok) {
                std::cerr << "读串口失败" << std::endl;
                continue;
            }

            if (*bytesRead == 0) {
                continue;
            }

            ofs.write(buf.data(), static_cast<std::streamsize>(*bytesRead));
            ofs.flush();

            std::cout.write(buf.data(), static_cast<std::streamsize>(*bytesRead));
            std::cout.flush();
        }
    });

    // 主线程：从终端读入文本，发送到串口
    std::string line;
    while (g_running.load() && std::getline(std::cin, line)) {
        line.push_back('\n');
        bool ok = ser->write(line, line.size());
        if (!ok) {
            std::cerr << "写串口失败" << std::endl;
        }
    }

    g_running = false;
    if (reader.joinable()) {
        reader.join();
    }
    ofs.close();

    // Serial 析构时会自动关闭串口
    return 0;
}

#include <atomic>
#include <csignal>
#include <fstream>
#include <iostream>
#include <string>
#include <thread>

#include "serial/serial.hpp"


static std::atomic<bool> g_running { true };

void handle_sigint(int) {
    g_running = false;
}

struct Options {
    std::string port    = "/dev/ttyACM0";
    uint32_t baud       = 115200;
    std::string outfile = "serial_out.txt";
    size_t chunk        = 256; // 每次读取的字节数
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

    // 配置串口
    auto ioContext = std::make_unique<qdriver::io::IoContext>();

    std::shared_ptr<qdriver::io::Serial> serialPortPtr = std::make_shared<qdriver::io::Serial>(
        std::move(ioContext),
        "/dev/QD4310-0",
        115200,
        8,
        qdriver::io::SerialPortBase::parity::none,
        qdriver::io::SerialPortBase::stop_bits::one
    );

    if (!serialPortPtr->isOpen()) {
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

    // 读线程：从串口读取数据并写入文件
    std::thread reader([&]() {
        std::string buf(opt.chunk, '\0');
        while (g_running.load()) {
            auto rc = serialPortPtr->read(buf, opt.chunk);
            if (!rc) {
                std::cerr << "读串口失败" << std::endl;
                continue;
            }
            // ofs.write(
            //     reinterpret_cast<const char*>(buf.data()),
            //     static_cast<std::streamsize>(buf.size())
            // );
            std::cout << std::string(buf.begin(), buf.end());
            // ofs.flush();
        }
    });

    // 主线程：从终端读入文本，发送到串口
    std::string line;
    while (g_running.load() && std::getline(std::cin, line)) {
        // 将输入行作为原始字节发送（包含换行）
        std::string out(line.begin(), line.end());
        out.push_back('\n');
        auto wc = serialPortPtr->write(out, out.size());
        if (!wc) {
            std::cerr << "写串口失败" << std::endl;
        }
    }

    g_running = false;
    if (reader.joinable())
        reader.join();
    ofs.close();

    return 0;
}

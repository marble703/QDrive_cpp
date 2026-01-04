#include "can.hpp"
#include "interface.hpp"

#include <iostream>

using namespace qdriver::io;
using namespace qdriver::interface;

static std::atomic<bool> g_running { true };

void handle_sigint(int) {
    g_running = false;
    std::cout << "\n\n已退出" << std::endl;
}

struct Options {
    std::string can_interface = "can0";
    int send_can_id           = 0x400;
    int receive_can_id        = 0x500;
};

void print_usage(const char* prog) {
    std::cout << "用法: " << prog << " [-i <interface>] [-s <send_id>] [-r <recv_id>]" << std::endl
              << "示例: " << prog << " -i can0 -s 0x400 -r 0x500" << std::endl;
}

bool parse_args(int argc, char** argv, Options& opt) {
    for (int i = 1; i < argc; ++i) {
        std::string a = argv[i];
        if ((a == "-h") || (a == "--help")) {
            print_usage(argv[0]);
            return false;
        }
        if ((a == "-i" || a == "--interface") && i + 1 < argc) {
            opt.can_interface = argv[++i];
            continue;
        }
        if ((a == "-s" || a == "--send") && i + 1 < argc) {
            opt.send_can_id = std::stoi(argv[++i], nullptr, 16);
            continue;
        }
        if ((a == "-r" || a == "--recv") && i + 1 < argc) {
            opt.receive_can_id = std::stoi(argv[++i], nullptr, 16);
            continue;
        }
        std::cerr << "未知参数: " << a << std::endl;
        print_usage(argv[0]);
        return false;
    }
    return true;
}

void print_help() {
    std::cout << "\n=== QDrive CAN 交互式控制工具 ===" << std::endl;
    std::cout << "命令列表:\n" << std::endl;
    std::cout << "  enable [can_id]          - 使能电机" << std::endl;
    std::cout << "  disable [can_id]         - 失能电机" << std::endl;
    std::cout << "  status [can_id]          - 查询电机状态" << std::endl;
    std::cout << "  current <value> [can_id] - 电流控制 (-10A ~ 10A)" << std::endl;
    std::cout << "  speed <value> [can_id]   - 速度控制 (-1000rpm ~ 1000rpm)" << std::endl;
    std::cout << "  angle <value> [can_id]   - 角度控制 (0 ~ 2π rad)" << std::endl;
    std::cout << "  lowspeed <value> [can_id]- 低速控制 (-1000rpm ~ 1000rpm)" << std::endl;
    std::cout << "  stepangle <value> [can_id] - 步进角度控制 (-5π ~ 5π rad)" << std::endl;
    std::cout << "  help                     - 显示此帮助信息" << std::endl;
    std::cout << "  exit/quit                - 退出程序" << std::endl;
    std::cout << "\n提示: Ctrl+C 也可以退出程序\n" << std::endl;
}

// 将输入字符串转换为小写
std::string to_lower(const std::string& str) {
    std::string result = str;
    std::transform(result.begin(), result.end(), result.begin(), [](unsigned char c) {
        return std::tolower(c);
    });
    return result;
}

// 分割字符串
std::vector<std::string> split_command(const std::string& line) {
    std::vector<std::string> tokens;
    std::istringstream iss(line);
    std::string token;
    while (iss >> token) {
        tokens.push_back(token);
    }
    return tokens;
}

int main(int argc, char** argv) {
    std::signal(SIGINT, handle_sigint);

    Options opt;
    if (!parse_args(argc, argv, opt)) {
        return 1;
    }

    // 创建 CAN 接口
    auto can_bus = std::make_shared<Can>(opt.can_interface, opt.send_can_id, opt.receive_can_id);
    if (!can_bus->isOpen()) {
        std::cerr << "打开CAN接口失败: " << opt.can_interface << std::endl;
        return 2;
    }

    auto interface = std::make_unique<Interface>(can_bus);

    std::cout << "CAN接口已打开: " << opt.can_interface << std::endl;
    std::cout << "发送CAN ID: 0x" << std::hex << opt.send_can_id << ", 接收CAN ID: 0x"
              << opt.receive_can_id << std::dec << std::endl;

    print_help();

    // 读取线程：后台接收CAN数据
    std::thread reader([&]() {
        std::vector<uint8_t> data;
        while (g_running.load()) {
            if (can_bus->receiveFrame(data)) {
                std::cout << "\n[接收] ";
                for (size_t i = 0; i < data.size(); ++i) {
                    std::cout << std::hex << std::setw(2) << std::setfill('0') << (int)data[i]
                              << " ";
                }
                std::cout << std::dec << std::endl;
                std::cout << "> ";
                std::cout.flush();
            }
        }
    });

    // 主交互循环
    std::string line;
    while (g_running.load()) {
        std::cout << "> ";
        std::cout.flush();

        if (!std::getline(std::cin, line)) {
            break;
        }

        // 去掉首尾空格
        line.erase(0, line.find_first_not_of(" \t\n\r\f\v"));
        line.erase(line.find_last_not_of(" \t\n\r\f\v") + 1);

        if (line.empty()) {
            continue;
        }

        auto tokens = split_command(line);
        if (tokens.empty()) {
            continue;
        }

        std::string cmd = to_lower(tokens[0]);

        try {
            if (cmd == "exit" || cmd == "quit") {
                g_running = false;
                break;
            } else if (cmd == "help") {
                print_help();
            } else if (cmd == "enable") {
                int can_id = -1;
                if (tokens.size() > 1) {
                    can_id = std::stoi(tokens[1], nullptr, 16);
                }
                if (interface->enable(can_id)) {
                    std::cout << "✓ 使能命令已发送" << std::endl;
                } else {
                    std::cout << "✗ 使能命令失败" << std::endl;
                }
            } else if (cmd == "disable") {
                int can_id = -1;
                if (tokens.size() > 1) {
                    can_id = std::stoi(tokens[1], nullptr, 16);
                }
                if (interface->disable(can_id)) {
                    std::cout << "✓ 失能命令已发送" << std::endl;
                } else {
                    std::cout << "✗ 失能命令失败" << std::endl;
                }
            } else if (cmd == "status") {
                int can_id = -1;
                if (tokens.size() > 1) {
                    can_id = std::stoi(tokens[1], nullptr, 16);
                }
                if (interface->status(can_id)) {
                    std::cout << "✓ 状态查询命令已发送" << std::endl;
                } else {
                    std::cout << "✗ 状态查询命令失败" << std::endl;
                }
            } else if (cmd == "current") {
                if (tokens.size() < 2) {
                    std::cout << "用法: current <value> [can_id]" << std::endl;
                    continue;
                }
                float current = std::stof(tokens[1]);
                int can_id    = -1;
                if (tokens.size() > 2) {
                    can_id = std::stoi(tokens[2], nullptr, 16);
                }
                if (interface->ctrlCurrent(current, can_id)) {
                    std::cout << "✓ 电流控制命令已发送 (电流: " << current << "A)" << std::endl;
                } else {
                    std::cout << "✗ 电流控制命令失败" << std::endl;
                }
            } else if (cmd == "speed") {
                if (tokens.size() < 2) {
                    std::cout << "用法: speed <value> [can_id]" << std::endl;
                    continue;
                }
                float speed = std::stof(tokens[1]);
                int can_id  = -1;
                if (tokens.size() > 2) {
                    can_id = std::stoi(tokens[2], nullptr, 16);
                }
                if (interface->ctrlSpeed(speed, can_id)) {
                    std::cout << "✓ 速度控制命令已发送 (速度: " << speed << " rpm)" << std::endl;
                } else {
                    std::cout << "✗ 速度控制命令失败" << std::endl;
                }
            } else if (cmd == "angle") {
                if (tokens.size() < 2) {
                    std::cout << "用法: angle <value> [can_id]" << std::endl;
                    continue;
                }
                float angle = std::stof(tokens[1]);
                int can_id  = -1;
                if (tokens.size() > 2) {
                    can_id = std::stoi(tokens[2], nullptr, 16);
                }
                if (interface->ctrlAngle(angle, can_id)) {
                    std::cout << "✓ 角度控制命令已发送 (角度: " << angle << " rad)" << std::endl;
                } else {
                    std::cout << "✗ 角度控制命令失败" << std::endl;
                }
            } else if (cmd == "lowspeed") {
                if (tokens.size() < 2) {
                    std::cout << "用法: lowspeed <value> [can_id]" << std::endl;
                    continue;
                }
                float speed = std::stof(tokens[1]);
                int can_id  = -1;
                if (tokens.size() > 2) {
                    can_id = std::stoi(tokens[2], nullptr, 16);
                }
                if (interface->ctrlLowSpeed(speed, can_id)) {
                    std::cout << "✓ 低速控制命令已发送 (速度: " << speed << " rpm)" << std::endl;
                } else {
                    std::cout << "✗ 低速控制命令失败" << std::endl;
                }
            } else if (cmd == "stepangle") {
                if (tokens.size() < 2) {
                    std::cout << "用法: stepangle <value> [can_id]" << std::endl;
                    continue;
                }
                float angle = std::stof(tokens[1]);
                int can_id  = -1;
                if (tokens.size() > 2) {
                    can_id = std::stoi(tokens[2], nullptr, 16);
                }
                if (interface->ctrlStepAngle(angle, can_id)) {
                    std::cout << "✓ 步进角度控制命令已发送 (角度: " << angle << " rad)"
                              << std::endl;
                } else {
                    std::cout << "✗ 步进角度控制命令失败" << std::endl;
                }
            } else {
                std::cout << "未知命令: " << cmd << " (输入 'help' 查看帮助)" << std::endl;
            }
        } catch (const std::exception& e) {
            std::cerr << "执行命令出错: " << e.what() << std::endl;
        }
    }

    g_running = false;
    if (reader.joinable()) {
        reader.join();
    }

    return 0;
}

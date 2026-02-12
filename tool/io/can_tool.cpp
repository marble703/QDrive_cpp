#include "can.hpp"
#include "interface.hpp"

#include <csignal>
#include <iostream>
#include <optional>

using namespace qdriver::io;
using namespace qdriver::interface;

static std::atomic<bool> g_running { true };

void handle_sigint(int) {
    g_running = false;
    std::cout << "\n\n已退出" << std::endl;
}

void print_usage(const char* prog) {
    std::cout << "用法: " << prog << " [-i <interface>]" << std::endl
              << "示例: " << prog << " -i can0" << std::endl;
}

bool parse_args(int argc, char** argv, std::string& can_interface) {
    for (int i = 1; i < argc; ++i) {
        std::string a = argv[i];
        if ((a == "-h") || (a == "--help")) {
            print_usage(argv[0]);
            return false;
        }
        if ((a == "-i" || a == "--interface") && i + 1 < argc) {
            can_interface = argv[++i];
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
    std::cout << "  help                     - 显示此帮助信息" << std::endl;
    std::cout << "  exit/quit                - 退出程序" << std::endl;
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

    std::string can_interface;
    if (!parse_args(argc, argv, can_interface)) {
        return 1;
    }
    std::cout << "CAN接口: " << can_interface << std::endl;

    // 创建 CAN 接口
    auto can_bus = std::make_shared<Can>(can_interface);
    if (!can_bus->isOpen()) {
        std::cerr << "打开CAN接口失败: " << can_interface << std::endl;
        return 2;
    }

    auto interface = std::make_unique<Interface>(can_bus);

    std::cout << "CAN接口已打开: " << can_interface << std::endl;

    print_help();

    // 读取线程：后台接收CAN数据
    std::thread reader([&]() {
        std::vector<uint8_t> data;
        std::shared_ptr<uint32_t> can_id = std::make_shared<uint32_t>(0);
        while (g_running.load()) {
            if (can_bus->receiveFrame(data, can_id)) {
                std::cout << "\n" << std::hex << std::setw(3) << std::setfill('0');

                // 打印原始数据
                for (size_t i = 0; i < data.size(); ++i) {
                    std::cout << std::hex << std::setw(2) << std::setfill('0')
                              << static_cast<int>(data[i]) << " ";
                }
                std::cout << std::dec << std::endl;

                // 按协议解析反馈报文: ID = 0x500 + 电机ID, DLC = 8
                int motor_id = static_cast<int>(*can_id) - 0x500;

                uint8_t status = data[0];
                bool enable    = (status & 0x01) != 0; // bit0 使能标志

                auto u16_be = [](uint8_t high, uint8_t low) {
                    return static_cast<int16_t>(
                        (static_cast<uint16_t>(high) << 8) | static_cast<uint16_t>(low)
                    );
                };

                int16_t current_raw = u16_be(data[3], data[2]);
                int16_t speed_raw   = u16_be(data[5], data[4]);
                int16_t angle_raw   = u16_be(data[7], data[6]);

                std::cout << "[解析] 电机ID: 0x" << std::hex << motor_id << std::dec
                          << "  角度(raw): " << angle_raw << "  转速(raw): " << speed_raw
                          << "  电流(raw): " << current_raw << "  使能: " << (enable ? "ON" : "OFF")
                          << std::endl;

                std::cout << "> ";
                std::cout.flush();
            }
        }
    });

    auto parse_can_id = [](const std::string& token) -> std::optional<uint32_t> {
        unsigned long parsed = std::stoul(token, nullptr, 16);
        if (parsed > std::numeric_limits<uint32_t>::max())
            return std::nullopt;
        return static_cast<uint32_t>(parsed);
    };

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
                std::optional<uint32_t> can_id;
                if (tokens.size() > 1) {
                    can_id = parse_can_id(tokens[1]);
                    if (!can_id.has_value())
                        throw std::out_of_range("can_id out of range");
                }
                if (can_id.has_value() ? interface->enable(*can_id) : interface->enable()) {
                    std::cout << "✓ 使能命令已发送" << std::endl;
                } else {
                    std::cout << "✗ 使能命令失败" << std::endl;
                }
            } else if (cmd == "disable") {
                std::optional<uint32_t> can_id;
                if (tokens.size() > 1) {
                    can_id = parse_can_id(tokens[1]);
                    if (!can_id.has_value())
                        throw std::out_of_range("can_id out of range");
                }
                if (can_id.has_value() ? interface->disable(*can_id) : interface->disable()) {
                    std::cout << "✓ 失能命令已发送" << std::endl;
                } else {
                    std::cout << "✗ 失能命令失败" << std::endl;
                }
            } else if (cmd == "status") {
                std::optional<uint32_t> can_id;
                if (tokens.size() > 1) {
                    can_id = parse_can_id(tokens[1]);
                    if (!can_id.has_value())
                        throw std::out_of_range("can_id out of range");
                }
                if (can_id.has_value() ? interface->status(*can_id) : interface->status()) {
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
                std::optional<uint32_t> can_id;
                if (tokens.size() > 2) {
                    can_id = parse_can_id(tokens[2]);
                    if (!can_id.has_value())
                        throw std::out_of_range("can_id out of range");
                }
                if (can_id.has_value() ? interface->ctrlCurrent(current, *can_id)
                                       : interface->ctrlCurrent(current)) {
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
                std::optional<uint32_t> can_id;
                if (tokens.size() > 2) {
                    can_id = parse_can_id(tokens[2]);
                    if (!can_id.has_value())
                        throw std::out_of_range("can_id out of range");
                }
                if (can_id.has_value() ? interface->ctrlSpeed(speed, *can_id)
                                       : interface->ctrlSpeed(speed)) {
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
                std::optional<uint32_t> can_id;
                if (tokens.size() > 2) {
                    can_id = parse_can_id(tokens[2]);
                    if (!can_id.has_value())
                        throw std::out_of_range("can_id out of range");
                }
                if (can_id.has_value() ? interface->ctrlAngle(angle, *can_id)
                                       : interface->ctrlAngle(angle)) {
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
                std::optional<uint32_t> can_id;
                if (tokens.size() > 2) {
                    can_id = parse_can_id(tokens[2]);
                    if (!can_id.has_value())
                        throw std::out_of_range("can_id out of range");
                }
                if (can_id.has_value() ? interface->ctrlLowSpeed(speed, *can_id)
                                       : interface->ctrlLowSpeed(speed)) {
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
                std::optional<uint32_t> can_id;
                if (tokens.size() > 2) {
                    can_id = parse_can_id(tokens[2]);
                    if (!can_id.has_value())
                        throw std::out_of_range("can_id out of range");
                }
                if (can_id.has_value() ? interface->ctrlStepAngle(angle, *can_id)
                                       : interface->ctrlStepAngle(angle)) {
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

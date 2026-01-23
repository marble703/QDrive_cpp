#pragma once

#include <fstream>
#include <string>

struct AppConfig {
    std::string serialPort   = "/dev/ttyACM0";
    std::string canInterface = "can0";
    int sendIdHex            = 0x400;
    int recvIdHex            = 0x500;
};

inline void saveConfig(const AppConfig& cfg, const std::string& filename = "qdrive_gui.conf") {
    std::ofstream f(filename);
    if (f.is_open()) {
        f << "serial_port=" << cfg.serialPort << "\n";
        f << "can_interface=" << cfg.canInterface << "\n";
        f << "send_id=0x" << std::hex << cfg.sendIdHex << "\n";
        f << "recv_id=0x" << std::hex << cfg.recvIdHex << "\n";
    }
}

inline AppConfig loadConfig(const std::string& filename = "qdrive_gui.conf") {
    AppConfig cfg;
    std::ifstream f(filename);
    if (f.is_open()) {
        std::string line;
        while (std::getline(f, line)) {
            auto pos = line.find('=');
            if (pos == std::string::npos) continue;
            std::string key = line.substr(0, pos);
            std::string val = line.substr(pos + 1);
            if (key == "serial_port") cfg.serialPort = val;
            else if (key == "can_interface") cfg.canInterface = val;
            else if (key == "send_id") cfg.sendIdHex = std::stoi(val, nullptr, 16);
            else if (key == "recv_id") cfg.recvIdHex = std::stoi(val, nullptr, 16);
        }
    }
    return cfg;
}

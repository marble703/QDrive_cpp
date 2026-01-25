#pragma once

#include <fstream>
#include <string>
#include "motor_controller.hpp"

struct AppConfig {
    std::string serialPort   = "/dev/ttyACM0";
    std::string canInterface = "can0";
    int sendIdHex            = 0x400;
    int recvIdHex            = 0x500;

    // Display ranges persisted for the UI
    float displayMinSpeed   = MIN_SPEED_CTRL_VALUE;
    float displayMaxSpeed   = MAX_SPEED_CTRL_VALUE;
    float displayMinAngle   = MIN_ANGLE_CTRL_VALUE;
    float displayMaxAngle   = MAX_ANGLE_CTRL_VALUE;
    float displayMinCurrent = MIN_CURRENT_CTRL_VALUE;
    float displayMaxCurrent = MAX_CURRENT_CTRL_VALUE;
};

inline void saveConfig(const AppConfig& cfg, const std::string& filename = "qdrive_gui.conf") {
    std::ofstream f(filename);
    if (f.is_open()) {
        f << "serial_port=" << cfg.serialPort << "\n";
        f << "can_interface=" << cfg.canInterface << "\n";
        f << "send_id=0x" << std::hex << cfg.sendIdHex << std::dec << "\n";
        f << "recv_id=0x" << std::hex << cfg.recvIdHex << std::dec << "\n";
        f << "display_min_speed=" << cfg.displayMinSpeed << "\n";
        f << "display_max_speed=" << cfg.displayMaxSpeed << "\n";
        f << "display_min_angle=" << cfg.displayMinAngle << "\n";
        f << "display_max_angle=" << cfg.displayMaxAngle << "\n";
        f << "display_min_current=" << cfg.displayMinCurrent << "\n";
        f << "display_max_current=" << cfg.displayMaxCurrent << "\n";
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
            else if (key == "display_min_speed") cfg.displayMinSpeed = std::stof(val);
            else if (key == "display_max_speed") cfg.displayMaxSpeed = std::stof(val);
            else if (key == "display_min_angle") cfg.displayMinAngle = std::stof(val);
            else if (key == "display_max_angle") cfg.displayMaxAngle = std::stof(val);
            else if (key == "display_min_current") cfg.displayMinCurrent = std::stof(val);
            else if (key == "display_max_current") cfg.displayMaxCurrent = std::stof(val);
        }
    }
    return cfg;
}

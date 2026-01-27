#pragma once

#include <fstream>

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

    // Error plot ranges persisted for the UI
    float errorMinSpeed   = -MAX_SPEED_CTRL_VALUE;
    float errorMaxSpeed   = MAX_SPEED_CTRL_VALUE;
    float errorMinAngle   = -PI;
    float errorMaxAngle   = PI;
    float errorMinCurrent = -MAX_CURRENT_CTRL_VALUE;
    float errorMaxCurrent = MAX_CURRENT_CTRL_VALUE;
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

        f << "error_min_speed=" << cfg.errorMinSpeed << "\n";
        f << "error_max_speed=" << cfg.errorMaxSpeed << "\n";
        f << "error_min_angle=" << cfg.errorMinAngle << "\n";
        f << "error_max_angle=" << cfg.errorMaxAngle << "\n";
        f << "error_min_current=" << cfg.errorMinCurrent << "\n";
        f << "error_max_current=" << cfg.errorMaxCurrent << "\n";
    }
}

inline AppConfig loadConfig(const std::string& filename = "qdrive_gui.conf") {
    AppConfig cfg;
    std::ifstream f(filename);
    if (f.is_open()) {
        std::string line;
        while (std::getline(f, line)) {
            auto pos = line.find('=');
            if (pos == std::string::npos)
                continue;
            std::string key = line.substr(0, pos);
            std::string val = line.substr(pos + 1);
            if (key == "serial_port")
                cfg.serialPort = val;
            else if (key == "can_interface")
                cfg.canInterface = val;
            else if (key == "send_id")
                cfg.sendIdHex = std::stoi(val, nullptr, 16);
            else if (key == "recv_id")
                cfg.recvIdHex = std::stoi(val, nullptr, 16);
            else if (key == "display_min_speed")
                cfg.displayMinSpeed = std::stof(val);
            else if (key == "display_max_speed")
                cfg.displayMaxSpeed = std::stof(val);
            else if (key == "display_min_angle")
                cfg.displayMinAngle = std::stof(val);
            else if (key == "display_max_angle")
                cfg.displayMaxAngle = std::stof(val);
            else if (key == "display_min_current")
                cfg.displayMinCurrent = std::stof(val);
            else if (key == "display_max_current")
                cfg.displayMaxCurrent = std::stof(val);
            else if (key == "error_min_speed")
                cfg.errorMinSpeed = std::stof(val);
            else if (key == "error_max_speed")
                cfg.errorMaxSpeed = std::stof(val);
            else if (key == "error_min_angle")
                cfg.errorMinAngle = std::stof(val);
            else if (key == "error_max_angle")
                cfg.errorMaxAngle = std::stof(val);
            else if (key == "error_min_current")
                cfg.errorMinCurrent = std::stof(val);
            else if (key == "error_max_current")
                cfg.errorMaxCurrent = std::stof(val);
        }
    }
    return cfg;
}

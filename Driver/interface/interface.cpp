#include "interface.hpp"

namespace qdriver::interface {
Interface::Interface(
    std::shared_ptr<qdriver::io::Serial> serialPort,
    std::shared_ptr<qdriver::logger::Logger> logger
): InterfaceBase(serialPort, logger) {
    logger_->info("[Interface] Created with SERIAL interface");
}

Interface::Interface(
    std::shared_ptr<qdriver::io::Can> canPort,
    std::shared_ptr<qdriver::logger::Logger> logger
): InterfaceBase(canPort, 0x400, 0x500, logger) {
    logger_->info("[Interface] Created with CAN interface");
}

bool Interface::help() {
    if (this->getIoType() == ioType::CAN) {
        throw std::runtime_error("Help command is not supported for CAN interface");
    }
    return this->sendCommand({ .cmd = "help" });
}

bool Interface::version() {
    if (this->getIoType() == ioType::CAN) {
        throw std::runtime_error("Version command is not supported for CAN interface");
    }
    return this->sendCommand({ .cmd = "version" });
}

bool Interface::info() {
    if (this->getIoType() == ioType::CAN) {
        throw std::runtime_error("Info command is not supported for CAN interface");
    }
    return this->sendCommand({ .cmd = "info" });
}

bool Interface::status(uint32_t canID) {
    if (this->getIoType() == ioType::CAN) {
        return this->sendCommand({ .id = canID, .ctrlCommand = 0x00, .ctrlValue = 0 });
    } else if (this->getIoType() == ioType::SERIAL) {
        return this->sendCommand({ .cmd = "status" });
    }
    return false;
}

bool Interface::enable(uint32_t canID) {
    logger_->info("[Interface] Enabling motor");
    if (this->getIoType() == ioType::CAN) {
        return this->sendCommand({ .id = canID, .ctrlCommand = 0x01, .ctrlValue = 1 });

    } else if (this->getIoType() == ioType::SERIAL) {
        return this->sendCommand({ .cmd = "enable" });
    }
    return false;
}

bool Interface::disable(uint32_t canID) {
    logger_->info("[Interface] Disabling motor");
    if (this->getIoType() == ioType::CAN) {
        return this->sendCommand({ .id = canID, .ctrlCommand = 0x02, .ctrlValue = 1 });

    } else if (this->getIoType() == ioType::SERIAL) {
        return this->sendCommand({ .cmd = "disable" });
    }
    return false;
}

bool Interface::silent() {
    if (this->getIoType() == ioType::CAN) {
        throw std::runtime_error("Silent command is not supported for CAN interface");
    }
    return this->sendCommand({ .cmd = "silent" });
}

bool Interface::reboot() {
    if (this->getIoType() == ioType::CAN) {
        throw std::runtime_error("Reboot command is not supported for CAN interface");
    }
    return this->sendCommand({ .cmd = "reboot" });
}

bool Interface::ctrlCurrent(float current, uint32_t canID) {
    logger_->debug("[Interface] Controlling current: {} A", current);
    if (this->getIoType() == ioType::CAN) {
        return this->sendCommand(
            { .id = canID, .ctrlCommand = 0x03, .ctrlValue = curentToCtrlValue(current) }
        );

    } else if (this->getIoType() == ioType::SERIAL) {
        return this->sendCommand(
            { .cmd = "ctrl", .parameter = { "current" }, .value = std::to_string(current) }
        );
    }
    return false;
}

bool Interface::ctrlSpeed(float speed, uint32_t canID) {
    logger_->debug("[Interface] Controlling speed: {} rpm", speed);
    if (this->getIoType() == ioType::CAN) {
        return this->sendCommand(
            { .id = canID, .ctrlCommand = 0x04, .ctrlValue = speedToCtrlValue(speed) }
        );

    } else if (this->getIoType() == ioType::SERIAL) {
        return this->sendCommand(
            { .cmd = "ctrl", .parameter = { "speed" }, .value = std::to_string(speed) }
        );
    }
    return false;
}

bool Interface::ctrlAngle(float angle, uint32_t canID) {
    if (this->getIoType() == ioType::CAN) {
        return this->sendCommand(
            { .id = canID, .ctrlCommand = 0x05, .ctrlValue = angleToCtrlValue(angle) }
        );

    } else if (this->getIoType() == ioType::SERIAL) {
        return this->sendCommand(
            { .cmd = "ctrl", .parameter = { "angle" }, .value = std::to_string(angle) }
        );
    }
    return false;
}

bool Interface::ctrlLowSpeed(float speed, uint32_t canID) {
    if (this->getIoType() == ioType::CAN) {
        return this->sendCommand(
            { .id = canID, .ctrlCommand = 0x06, .ctrlValue = speedToCtrlValue(speed) }
        );

    } else if (this->getIoType() == ioType::SERIAL) {
        return this->sendCommand(
            { .cmd = "ctrl", .parameter = { "speed" }, .value = std::to_string(speed) }
        );
    }
    return false;
}

bool Interface::ctrlStepAngle(float angle, uint32_t canID) {
    if (this->getIoType() == ioType::CAN) {
        return this->sendCommand(
            { .id = canID, .ctrlCommand = 0x05, .ctrlValue = speedToCtrlValue(angle) }
        );

    } else if (this->getIoType() == ioType::SERIAL) {
        return this->sendCommand(
            { .cmd = "ctrl", .parameter = { "angle" }, .value = std::to_string(angle) }
        );
    }
    return false;
}

bool Interface::store() {
    if (this->getIoType() == ioType::CAN) {
        throw std::runtime_error("Store command is not supported for CAN interface");
    }
    return this->sendCommand({ .cmd = "store" });
}

bool Interface::restore() {
    if (this->getIoType() == ioType::CAN) {
        throw std::runtime_error("Restore command is not supported for CAN interface");
    }
    return this->sendCommand({ .cmd = "restore" });
}

bool Interface::configSpeed(float value, PIDtype pidType) {
    if (this->getIoType() == ioType::CAN) {
        throw std::runtime_error("ConfigSpeed command is not supported for CAN interface");
    }

    std::string pidStr;
    switch (pidType) {
        case PIDtype::KP:
            pidStr = "kp";
            break;
        case PIDtype::KI:
            pidStr = "ki";
            break;
        case PIDtype::KD:
            pidStr = "kd";
            break;
        default:
            throw std::runtime_error("Unknown PIDtype type");
    }

    return this->sendCommand(
        { .cmd = "config", .parameter = " pid.speed." + pidStr, .value = std::to_string(value) }
    );
}

bool Interface::configAngle(float value, PIDtype pidType) {
    if (this->getIoType() == ioType::CAN) {
        throw std::runtime_error("ConfigAngle command is not supported for CAN interface");
    }

    std::string pidStr;
    switch (pidType) {
        case PIDtype::KP:
            pidStr = "kp";
            break;
        case PIDtype::KI:
            pidStr = "ki";
            break;
        case PIDtype::KD:
            pidStr = "kd";
            break;
        default:
            throw std::runtime_error("Unknown PIDtype type");
    }

    return this->sendCommand(
        { .cmd = "config", .parameter = " pid.angle." + pidStr, .value = std::to_string(value) }
    );
}

bool Interface::configLimitSpeed(float speed) {
    if (this->getIoType() == ioType::CAN) {
        throw std::runtime_error("ConfigLimitSpeed command is not supported for CAN interface");
    }
    return this->sendCommand(
        { .cmd = "config", .parameter = " limit.speed", .value = std::to_string(speed) }
    );
}

bool Interface::configLimitCurrent(float current) {
    if (this->getIoType() == ioType::CAN) {
        throw std::runtime_error("ConfigLimitCurrent command is not supported for CAN interface");
    }
    return this->sendCommand(
        { .cmd = "config", .parameter = " limit.current", .value = std::to_string(current) }
    );
}

bool Interface::configCanID(uint32_t canID) {
    if (this->getIoType() == ioType::CAN) {
        throw std::runtime_error("ConfigCanID command is not supported for CAN interface");
    }
    return this->sendCommand(
        { .cmd = "config", .parameter = " can.id", .value = std::to_string(canID) }
    );
}

bool Interface::configBaudRate(unsigned int baudRate) {
    if (this->getIoType() == ioType::CAN) {
        throw std::runtime_error("ConfigBaudRate command is not supported for CAN interface");
    }
    return this->sendCommand(
        { .cmd = "config", .parameter = " baudrate", .value = std::to_string(baudRate) }
    );
}

bool Interface::getConfig() {
    if (this->getIoType() == ioType::CAN) {
        throw std::runtime_error("ConfigBaudRate command is not supported for CAN interface");
    }
    return this->sendCommand(
        { .cmd = "config", .parameter = " --list", .value = "" }
    );
}

int16_t curentToCtrlValue(float current)  {
    float scaled = (std::clamp(current, MIN_CURRENT_CTRL_VALUE, MAX_CURRENT_CTRL_VALUE)
                    - MIN_CURRENT_CTRL_VALUE)
            / (MAX_CURRENT_CTRL_VALUE - MIN_CURRENT_CTRL_VALUE) * 65535.0f
        - 32768.0f;
    return boost::numeric_cast<int16_t>(scaled);
}

int16_t speedToCtrlValue(float speed)  {
    float scaled =
        (std::clamp(speed, MIN_SPEED_CTRL_VALUE, MAX_SPEED_CTRL_VALUE) - MIN_SPEED_CTRL_VALUE)
            / (MAX_SPEED_CTRL_VALUE - MIN_SPEED_CTRL_VALUE) * 65535.0f
        - 32768.0f;
    return boost::numeric_cast<int16_t>(scaled);
}

int16_t angleToCtrlValue(float angle)  {
    float scaled =
        (std::clamp(angle, MIN_ANGLE_CTRL_VALUE, MAX_ANGLE_CTRL_VALUE) - MIN_ANGLE_CTRL_VALUE)
            / (MAX_ANGLE_CTRL_VALUE - MIN_ANGLE_CTRL_VALUE) * 65535.0f
        - 32768.0f;
    return boost::numeric_cast<int16_t>(scaled);
}

int16_t stepAngleToCtrlValue(float angle)  {
    float scaled = (std::clamp(angle, MIN_STEPANGLE_CTRL_VALUE, MAX_STEPANGLE_CTRL_VALUE)
                    - MIN_STEPANGLE_CTRL_VALUE)
            / (MAX_STEPANGLE_CTRL_VALUE - MIN_STEPANGLE_CTRL_VALUE) * 65535.0f
        - 32768.0f;
    return boost::numeric_cast<int16_t>(scaled);
}

float ctrlValueToCurrent(int16_t ctrlValue) {
    int32_t v = std::clamp<int32_t>(static_cast<int32_t>(ctrlValue), -32768, 32767);
    float scaled = (static_cast<float>(v) + 32768.0f) / 65535.0f;
    return scaled * (MAX_CURRENT_CTRL_VALUE - MIN_CURRENT_CTRL_VALUE) + MIN_CURRENT_CTRL_VALUE;
}

float ctrlValueToSpeed(int16_t ctrlValue) {
    int32_t v = std::clamp<int32_t>(static_cast<int32_t>(ctrlValue), -32768, 32767);
    float scaled = (static_cast<float>(v) + 32768.0f) / 65535.0f;
    return scaled * (MAX_SPEED_CTRL_VALUE - MIN_SPEED_CTRL_VALUE) + MIN_SPEED_CTRL_VALUE;
}

float ctrlValueToAngle(int16_t ctrlValue) {
    int32_t v = std::clamp<int32_t>(static_cast<int32_t>(ctrlValue), -32768, 32767);
    float scaled = (static_cast<float>(v) + 32768.0f) / 65535.0f;
    return scaled * (MAX_ANGLE_CTRL_VALUE - MIN_ANGLE_CTRL_VALUE) + MIN_ANGLE_CTRL_VALUE;
}

float ctrlValueToStepAngle(int16_t ctrlValue) {
    int32_t v = std::clamp<int32_t>(static_cast<int32_t>(ctrlValue), -32768, 32767);
    float scaled = (static_cast<float>(v) + 32768.0f) / 65535.0f;
    return scaled * (MAX_STEPANGLE_CTRL_VALUE - MIN_STEPANGLE_CTRL_VALUE) + MIN_STEPANGLE_CTRL_VALUE;
}

} // namespace qdriver::interface
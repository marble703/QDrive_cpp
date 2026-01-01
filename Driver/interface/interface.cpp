#include "interface.hpp"
#include <algorithm>

namespace qdriver::interface {
Interface::Interface(std::shared_ptr<qdriver::io::Serial> serialPort): InterfaceBase(serialPort) {}
Interface::Interface(std::shared_ptr<qdriver::io::Can> canPort, uint32_t defalutSendCanID):
    InterfaceBase(canPort, defalutSendCanID) {}

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

bool Interface::status(int canID) {
    if (this->getIoType() == ioType::CAN) {
        return this->sendCommand(
            { .id = this->externalCanID(canID), .ctrlCommand = 0x00, .ctrlValue = 1 }
        );
    } else if (this->getIoType() == ioType::SERIAL) {
        return this->sendCommand({ .cmd = "enable" });
    }
    return false;
}

bool Interface::enable(int canID) {
    if (this->getIoType() == ioType::CAN) {
        return this->sendCommand(
            { .id = this->externalCanID(canID), .ctrlCommand = 0x01, .ctrlValue = 1 }
        );

    } else if (this->getIoType() == ioType::SERIAL) {
        return this->sendCommand({ .cmd = "enable" });
    }
    return false;
}

bool Interface::disable(int canID) {
    if (this->getIoType() == ioType::CAN) {
        return this->sendCommand(
            { .id = this->externalCanID(canID), .ctrlCommand = 0x02, .ctrlValue = 1 }
        );

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

bool Interface::ctrlCurrent(float current, int canID) {
    if (this->getIoType() == ioType::CAN) {
        return this->sendCommand(
            { .id          = this->externalCanID(canID),
              .ctrlCommand = 0x03,
              .ctrlValue   = this->curentToCtrlValue(current) }
        );

    } else if (this->getIoType() == ioType::SERIAL) {
        return this->sendCommand(
            { .cmd = "ctrl", .parameter = { "current" }, .value = std::to_string(current) }
        );
    }
    return false;
}

bool Interface::ctrlSpeed(float speed, int canID) {
    if (this->getIoType() == ioType::CAN) {
        return this->sendCommand(
            { .id          = this->externalCanID(canID),
              .ctrlCommand = 0x04,
              .ctrlValue   = this->speedToCtrlValue(speed) }
        );

    } else if (this->getIoType() == ioType::SERIAL) {
        return this->sendCommand(
            { .cmd = "ctrl", .parameter = { "speed" }, .value = std::to_string(speed) }
        );
    }
    return false;
}

bool Interface::ctrlAngle(float angle, int canID) {
    if (this->getIoType() == ioType::CAN) {
        return this->sendCommand(
            { .id          = this->externalCanID(canID),
              .ctrlCommand = 0x05,
              .ctrlValue   = this->angleToCtrlValue(angle) }
        );

    } else if (this->getIoType() == ioType::SERIAL) {
        return this->sendCommand(
            { .cmd = "ctrl", .parameter = { "angle" }, .value = std::to_string(angle) }
        );
    }
    return false;
}

bool Interface::ctrlLowSpeed(float speed, int canID) {
    if (this->getIoType() == ioType::CAN) {
        return this->sendCommand(
            { .id          = this->externalCanID(canID),
              .ctrlCommand = 0x06,
              .ctrlValue   = this->speedToCtrlValue(speed) }
        );

    } else if (this->getIoType() == ioType::SERIAL) {
        return this->sendCommand(
            { .cmd = "ctrl", .parameter = { "speed" }, .value = std::to_string(speed) }
        );
    }
    return false;
}

bool Interface::ctrlStepAngle(float angle, int canID) {
    if (this->getIoType() == ioType::CAN) {
        return this->sendCommand(
            { .id          = this->externalCanID(canID),
              .ctrlCommand = 0x05,
              .ctrlValue   = this->speedToCtrlValue(angle) }
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

uint32_t Interface::externalCanID(int canID) const {
    if (canID < 0) {
        return boost::numeric_cast<uint32_t>(this->defalutSendCanID);
    } else {
        return boost::numeric_cast<uint32_t>(canID);
    }
}

int16_t Interface::curentToCtrlValue(float current) const {
    float scaled = (std::clamp(current, MIN_CURRENT_CTRL_VALUE, MAX_CURRENT_CTRL_VALUE)
                    - MIN_CURRENT_CTRL_VALUE)
            / (MAX_CURRENT_CTRL_VALUE - MIN_CURRENT_CTRL_VALUE) * 65535.0f
        - 32768.0f;
    return boost::numeric_cast<int16_t>(scaled);
}

int16_t Interface::speedToCtrlValue(float speed) const {
    float scaled =
        (std::clamp(speed, MIN_SPEED_CTRL_VALUE, MAX_SPEED_CTRL_VALUE) - MIN_SPEED_CTRL_VALUE)
            / (MAX_SPEED_CTRL_VALUE - MIN_SPEED_CTRL_VALUE) * 65535.0f
        - 32768.0f;
    return boost::numeric_cast<int16_t>(scaled);
}

int16_t Interface::angleToCtrlValue(float angle) const {
    float scaled =
        (std::clamp(angle, MIN_ANGLE_CTRL_VALUE, MAX_ANGLE_CTRL_VALUE) - MIN_ANGLE_CTRL_VALUE)
            / (MAX_ANGLE_CTRL_VALUE - MIN_ANGLE_CTRL_VALUE) * 65535.0f
        - 32768.0f;
    return boost::numeric_cast<int16_t>(scaled);
}

int16_t Interface::stepAngleToCtrlValue(float angle) const {
    float scaled = (std::clamp(angle, MIN_STEPANGLE_CTRL_VALUE, MAX_STEPANGLE_CTRL_VALUE)
                    - MIN_STEPANGLE_CTRL_VALUE)
            / (MAX_STEPANGLE_CTRL_VALUE - MIN_STEPANGLE_CTRL_VALUE) * 65535.0f
        - 32768.0f;
    return boost::numeric_cast<int16_t>(scaled);
}

} // namespace qdriver::interface
#include "interface.hpp"

namespace qdriver::interface {
Interface::Interface(std::shared_ptr<qdriver::io::Serial> serialPort): InterfaceBase(serialPort) {}
Interface::Interface(std::shared_ptr<qdriver::io::Can> canPort): InterfaceBase(canPort) {}

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
bool Interface::status() {
    if (this->getIoType() == ioType::CAN) {
        throw std::runtime_error("Status command is not supported for CAN interface");
    }
    return this->sendCommand({ .cmd = "status" });
}

} // namespace qdriver::interface
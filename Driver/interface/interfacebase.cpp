#include "interfacebase.hpp"

namespace qdriver::interface {

InterfaceBase::InterfaceBase(std::shared_ptr<qdriver::io::Serial> serialPort):
    ioType_(ioType::SERIAL),
    serialPortPtr_(serialPort),
    canBusPtr_(nullptr) {}

InterfaceBase::~InterfaceBase() {
    this->stopReaderThread_.store(true);
    if (this->readerThread_.joinable()) {
        this->readerThread_.join();
    }
}

ioType InterfaceBase::getIoType(std::shared_ptr<std::string> ioTypeName) const {
    if (ioTypeName) {
        switch (this->ioType_) {
            case ioType::SERIAL:
                *ioTypeName = "SERIAL";
                break;
            case ioType::CAN:
                *ioTypeName = "CAN";
                break;
            default:
                *ioTypeName = "UNKNOWN";
                break;
        }
    }
    return this->ioType_;
}

bool InterfaceBase::sendCommand(const Command& command) {
    switch (this->ioType_) {
        case ioType::SERIAL:
            if (this->serialPortPtr_) {
                std::string fullCommand = command.cmd;
                if (!command.parameter.empty()) {
                    fullCommand += " " + command.parameter;
                }
                if (!command.value.empty()) {
                    fullCommand += " " + command.value;
                }
                fullCommand += "\n";
                return this->serialPortPtr_->write(fullCommand, fullCommand.size());
            }
            break;
        case ioType::CAN:
            // 还没写
            break;
        default:
            break;
    }

    return false;
}

bool InterfaceBase::startReaderThread(std::function<void(std::string&)> readerFunction) {
    
    if (this->readerThread_.joinable()) {
        return false; // 已经有线程在运行
    }

    this->readerThread_ = std::thread([this, readerFunction]() {

        std::string buffer(32, '\0');
        std::size_t pos;
        while (true) {
            this->stopReaderThread_.load();
            auto rc = serialPortPtr_->read(buffer, 32);
            if (!rc) {
                continue;
            }

            if (buffer.size() > 0) {
                readerFunction(buffer);
            }

            std::fill(buffer.begin(), buffer.end(), '\0');
        }
    });

    if(!this->readerThread_.joinable()) {
        return false; // 线程创建失败
    }
    this->readerThread_.detach();

    return true;
}

bool InterfaceBase::isSerialPortOpen() const {
    return this->ioType_ == ioType::SERIAL && this->serialPortPtr_->isOpen();
}

} // namespace qdriver::interface
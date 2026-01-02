#include "interfacebase.hpp"
namespace qdriver::interface {

InterfaceBase::InterfaceBase(std::shared_ptr<qdriver::io::Serial> serialPort):
    ioType_(ioType::SERIAL),
    serialPortPtr_(serialPort),
    canBusPtr_(nullptr) {}

InterfaceBase::InterfaceBase(
    std::shared_ptr<qdriver::io::Can> canPort,
    uint32_t sendCanID,
    uint32_t recvCanID
):
    ioType_(ioType::CAN),
    serialPortPtr_(nullptr),
    canBusPtr_(canPort),
    sendCanID_(sendCanID),
    recvCanID_(recvCanID) {}

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

bool InterfaceBase::sendCommand(const SerialCommand& command) {
    if (this->ioType_ == ioType::SERIAL) {
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
        } else {
            throw std::runtime_error("Serial port is not initialized");
        }
    } else if (this->ioType_ == ioType::CAN) {
        throw std::runtime_error("Illgal command for CAN interface");
    } else {
        throw std::runtime_error("Unknown IO type");
    }

    return false;
}

bool InterfaceBase::sendCommand(const CanCommand& command) {
    if (this->ioType_ == ioType::CAN) {
        if (this->canBusPtr_) {
            std::vector<uint8_t> data;
            data.push_back(command.ctrlCommand);
            data.push_back(static_cast<uint8_t>(command.ctrlValue & 0xFF));
            data.push_back(static_cast<uint8_t>((command.ctrlValue >> 8) & 0xFF));

            return this->canBusPtr_->sendFrame(data, command.id);
        } else {
            throw std::runtime_error("CAN port is not initialized");
        }
    } else if (this->ioType_ == ioType::SERIAL) {
        throw std::runtime_error("Illgal command for SERIAL interface");
    } else {
        throw std::runtime_error("Unknown IO type");
    }

    return false;
}

bool InterfaceBase::startReaderThread(std::function<void(std::string&)> readerFunction) {
    if (this->readerThread_.joinable()) {
        return false; // 已经有线程在运行
    }

    this->readerThread_ = std::thread([this, readerFunction]() {
        if (this->ioType_ == ioType::SERIAL) {
            // Todo: 检查缓冲区大小是否合理
            std::string buffer(32, '\0');
            while (true) {
                if (this->stopReaderThread_.load())
                    break;

                auto rc = serialPortPtr_->read(buffer, 32);
                if (!rc) {
                    continue;
                }

                if (buffer.size() > 0) {
                    readerFunction(buffer);
                }

                std::fill(buffer.begin(), buffer.end(), '\0');
            }
        } // 当前会监听整个 CAN 总线的数据 
        else if (this->ioType_ == ioType::CAN) {
            std::vector<uint8_t> data;
            while (true) {
                if (this->stopReaderThread_.load())
                    break;

                if (this->canBusPtr_->receiveFrame(data)) {
                    std::string buffer;
                    char hex[3];
                    for (auto b: data) {
                        std::snprintf(hex, sizeof(hex), "%02X", b);
                        buffer += hex;
                    }
                    readerFunction(buffer);
                }
            }
        }
    });

    if (!this->readerThread_.joinable()) {
        return false; // 线程创建失败
    }
    this->readerThread_.detach();

    return true;
}

bool InterfaceBase::isPortOpen() const {
    return this->ioType_ == ioType::SERIAL && this->serialPortPtr_->isOpen()
        || this->ioType_ == ioType::CAN && this->canBusPtr_->isOpen();
}

} // namespace qdriver::interface

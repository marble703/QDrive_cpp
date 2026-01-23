#include "interfacebase.hpp"
namespace qdriver::interface {

InterfaceBase::InterfaceBase(
    std::shared_ptr<qdriver::io::Serial> serialPort,
    std::shared_ptr<qdriver::logger::Logger> logger
):
    ioType_(ioType::SERIAL),
    serialPortPtr_(serialPort),
    canBusPtr_(nullptr),
    logger_(logger ? logger : qdriver::logger::LoggerFactory::getDefaultLogger()) {
    logger_->info("[InterfaceBase] Initialized with SERIAL interface");
}

InterfaceBase::InterfaceBase(
    std::shared_ptr<qdriver::io::Can> canPort,
    uint32_t sendCanID,
    uint32_t recvCanID,
    std::shared_ptr<qdriver::logger::Logger> logger
):
    ioType_(ioType::CAN),
    serialPortPtr_(nullptr),
    canBusPtr_(canPort),
    sendCanID_(sendCanID),
    recvCanID_(recvCanID),
    logger_(logger ? logger : qdriver::logger::LoggerFactory::getDefaultLogger()) {
    logger_->info(
        "[InterfaceBase] Initialized with CAN interface: sendID=0x{:X}, recvID=0x{:X}",
        sendCanID_,
        recvCanID_
    );
}

InterfaceBase::~InterfaceBase() {
    logger_->debug("[InterfaceBase] Destroying, stopping reader thread");
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
            logger_->debug("[InterfaceBase] Sending serial command: {}", fullCommand);
            bool result = this->serialPortPtr_->write(fullCommand, fullCommand.size());
            if (!result) {
                logger_->error("[InterfaceBase] Failed to send serial command");
            }
            return result;
        } else {
            logger_->error("[InterfaceBase] Serial port is not initialized");
            throw std::runtime_error("Serial port is not initialized");
        }
    } else if (this->ioType_ == ioType::CAN) {
        logger_->error("[InterfaceBase] Illegal command for CAN interface");
        throw std::runtime_error("Illgal command for CAN interface");
    } else {
        logger_->error("[InterfaceBase] Unknown IO type");
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

            logger_->debug(
                "[InterfaceBase] Sending CAN command: ID=0x{:X}, cmd=0x{:X}, value={}",
                command.id,
                command.ctrlCommand,
                command.ctrlValue
            );
            bool result = this->canBusPtr_->sendFrame(data, command.id);
            if (!result) {
                logger_->error("[InterfaceBase] Failed to send CAN command");
            }
            return result;
        } else {
            logger_->error("[InterfaceBase] CAN port is not initialized");
            throw std::runtime_error("CAN port is not initialized");
        }
    } else if (this->ioType_ == ioType::SERIAL) {
        logger_->error("[InterfaceBase] Illegal command for SERIAL interface");
        throw std::runtime_error("Illgal command for SERIAL interface");
    } else {
        logger_->error("[InterfaceBase] Unknown IO type");
        throw std::runtime_error("Unknown IO type");
    };

    return false;
}

bool InterfaceBase::startReaderThread(std::function<void(std::string&)> readerFunction) {
    if (this->readerThread_.joinable()) {
        logger_->warn("[InterfaceBase] Reader thread already running");
        return false; // 已经有线程在运行
    }

    logger_->info("[InterfaceBase] Starting reader thread");
    this->readerThread_ = std::thread([this, readerFunction]() {
        if (this->ioType_ == ioType::SERIAL) {
            logger_->debug("[InterfaceBase] Reader thread started for SERIAL");
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
            logger_->debug("[InterfaceBase] Reader thread stopped for SERIAL");
        } // 当前会监听整个 CAN 总线的数据
        else if (this->ioType_ == ioType::CAN)
        {
            logger_->debug("[InterfaceBase] Reader thread started for CAN");
            std::vector<uint8_t> data;
            auto canIdPtr = std::make_shared<size_t>();
            while (true) {
                if (this->stopReaderThread_.load())
                    break;

                if (this->canBusPtr_->receiveFrame(data, canIdPtr)) {
                    std::string buffer;
                    buffer += std::to_string(*canIdPtr);
                    buffer += ":";

                    char hex[3];
                    for (auto b: data) {
                        std::snprintf(hex, sizeof(hex), "%02X", b);
                        buffer += hex;
                    }
                    readerFunction(buffer);
                }
            }
            logger_->debug("[InterfaceBase] Reader thread stopped for CAN");
        }
    });

    if (!this->readerThread_.joinable()) {
        logger_->error("[InterfaceBase] Failed to create reader thread");
        return false; // 线程创建失败
    }
    this->readerThread_.detach();

    return true;
}

bool InterfaceBase::ReleaseReaderThread() {
    this->stopReaderThread_.store(true);
    if (this->readerThread_.joinable()) {
        this->readerThread_.join();
    }
    return true;
}

bool InterfaceBase::isPortOpen() const {
    return this->ioType_ == ioType::SERIAL && this->serialPortPtr_->isOpen()
        || this->ioType_ == ioType::CAN && this->canBusPtr_->isOpen();
}

} // namespace qdriver::interface

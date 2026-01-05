#pragma once

#include "can.hpp"
#include "serial.hpp"

#include "logger.hpp"

namespace qdriver::interface {

static const size_t LOW_SEND_CAN_ID  = 0x400;
static const size_t HIGH_SEND_CAN_ID = 0x40F;
static const size_t LOW_RECV_CAN_ID  = 0x500;
static const size_t HIGH_RECV_CAN_ID = 0x50F;

enum class ioType { SERIAL, CAN, NONE };

struct SerialCommand {
    std::string cmd       = "\n";          // 命令
    std::string parameter = std::string(); // 参数
    std::string value     = std::string(); // 值
};

struct CanCommand {
    uint32_t id         = 0x400;
    uint8_t ctrlCommand = 0x00;
    int16_t ctrlValue   = 0;
};

class InterfaceBase {
public:
    InterfaceBase(
        std::shared_ptr<qdriver::io::Serial> serialPort,
        std::shared_ptr<qdriver::logger::Logger> logger =
            std::make_shared<qdriver::logger::Logger>()
    );
    InterfaceBase(
        std::shared_ptr<qdriver::io::Can> canPort,
        uint32_t sendCanID = 0x400,
        uint32_t recvCanID = 0x500,
        std::shared_ptr<qdriver::logger::Logger> logger =
            std::make_shared<qdriver::logger::Logger>()
    );

    ~InterfaceBase();

    ioType getIoType(std::shared_ptr<std::string> ioTypeName = nullptr) const;

    bool isPortOpen() const;

    // 串口命令发送
    bool sendCommand(const SerialCommand& command);

    // CAN 命令发送
    bool sendCommand(const CanCommand& command);

    bool startReaderThread(std::function<void(std::string&)> readerFunction);

    bool ReleaseReaderThread();

protected:
    const ioType ioType_;

    std::shared_ptr<qdriver::io::Serial> serialPortPtr_;
    std::shared_ptr<qdriver::io::Can> canBusPtr_;

    std::atomic<bool> stopReaderThread_ { false };
    std::thread readerThread_;

    const uint32_t sendCanID_ = 0x400;
    const uint32_t recvCanID_ = 0x500;

    std::shared_ptr<qdriver::logger::Logger> logger_;
};

} // namespace qdriver::interface
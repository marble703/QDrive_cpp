#include "../io/can/can.hpp"
#include "../io/serial/serial.hpp"

#include <cstdint>
#include <thread>

namespace qdriver::interface {

enum class ioType {
    SERIAL,
    CAN,
};

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
    InterfaceBase(std::shared_ptr<qdriver::io::Serial> serialPort);
    InterfaceBase(std::shared_ptr<qdriver::io::Can> canPort);

    ~InterfaceBase();

    ioType getIoType(std::shared_ptr<std::string> ioTypeName = nullptr) const;

    // 串口命令发送
    bool sendCommand(const SerialCommand& command);

    bool sendCommand(const CanCommand& command);

    bool startReaderThread(std::function<void(std::string&)> readerFunction);

    bool isPortOpen() const;

protected:
    const ioType ioType_;

    std::shared_ptr<qdriver::io::Serial> serialPortPtr_;
    std::shared_ptr<qdriver::io::Can> canBusPtr_;

    std::atomic<bool> stopReaderThread_ { false };
    std::thread readerThread_;
};

} // namespace qdriver::interface
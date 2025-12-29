#include "../io/can/can.hpp"
#include "../io/serial/serial.hpp"

#include <thread>

namespace qdriver::interface {

enum class ioType {
    SERIAL,
    CAN,
};

struct Command {
    std::string cmd;
    std::string parameter = std::string();
    std::string value     = std::string();
};

class InterfaceBase {
public:
    InterfaceBase(std::shared_ptr<qdriver::io::Serial> serialPort);
    InterfaceBase(std::shared_ptr<qdriver::io::Can> canPort) = delete; // 还没写 CAN

    ~InterfaceBase();

    ioType getIoType(std::shared_ptr<std::string> ioTypeName = nullptr) const;

    bool sendCommand(const Command& command);

    bool startReaderThread(std::function<void(std::string&)> readerFunction);

    bool isSerialPortOpen() const;

protected:
    const ioType ioType_;

    std::shared_ptr<qdriver::io::Serial> serialPortPtr_;
    std::shared_ptr<qdriver::io::Can> canBusPtr_;

    std::atomic<bool> stopReaderThread_ { false };
    std::thread readerThread_;
};

} // namespace qdriver::interface
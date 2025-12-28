#include "../io/can/can.hpp"
#include "../io/serial/serial.hpp"

#include <functional>
#include <memory>
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

class Interface {
public:
    Interface(std::shared_ptr<qdriver::io::Serial> serialPort);
    ~Interface();

    ioType getIoType(std::shared_ptr<std::string> ioTypeName = nullptr) const;

    bool sendCommand(const Command& command);

    bool startReaderThread(std::function<void(std::string&)> readerFunction);

    bool isSerialPortOpen() const;

protected:
    const ioType ioType_;

private:
    std::shared_ptr<qdriver::io::Serial> serialPortPtr_;
    std::shared_ptr<qdriver::io::Can> canBusPtr_;

    std::atomic<bool> stopReaderThread_ { false };
    std::thread readerThread_;
};

} // namespace qdriver::interface
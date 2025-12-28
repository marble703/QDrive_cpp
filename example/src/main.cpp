#include "interface.hpp"
#include "serial/serial.hpp"

#include <iostream>

int main() {
    auto ioContext = std::make_unique<qdriver::io::IoContext>();

    std::shared_ptr<qdriver::io::Serial> serialPortPtr = std::make_shared<qdriver::io::Serial>(
        std::move(ioContext),
        "/dev/QD4310-0",
        115200,
        8,
        qdriver::io::SerialPortBase::parity::none,
        qdriver::io::SerialPortBase::stop_bits::one
    );

    qdriver::interface::Interface interface(serialPortPtr);


    if (interface.isSerialPortOpen()) {
        std::cout << "Serial port opened successfully." << std::endl;
    } else {
        std::cout << "Failed to open serial port." << std::endl;
        return 0;
    }

    interface.sendCommand({ .cmd = "enable" });
    sleep(1);
    interface.sendCommand({ .cmd = "ctrl", .parameter = "angle", .value = "3" });
    sleep(1);
    interface.sendCommand({ .cmd = "ctrl", .parameter = "angle", .value = "0" });
    sleep(1);
    interface.sendCommand({ .cmd = "disable" });
    sleep(1);

    return 0;
}
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

    while (true) {
        interface.sendCommand({ .cmd = "ctrl", .parameter = "step_angle", .value = "0.01" });
        std::this_thread::sleep_for(std::chrono::milliseconds(30));
    }

    return 0;
}
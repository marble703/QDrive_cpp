#include "base/interfacebase.hpp"
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

    qdriver::interface::InterfaceBase interface(serialPortPtr);

    if (interface.isPortOpen()) {
        std::cout << "port opened successfully." << std::endl;
    } else {
        std::cout << "Failed to open port." << std::endl;
    }

    interface.startReaderThread([](std::string& buffer) {
        std::cout << buffer << std::flush;
        return;
    });

    while (true) {
        sleep(1);
    }

    return 0;
}
#include "interface.hpp"
#include "serial.hpp"

#include <chrono>
#include <iostream>
#include <thread>

int main() {
    std::shared_ptr<qdriver::io::Serial> serialPortPtr = std::make_shared<qdriver::io::Serial>(
        std::make_unique<qdriver::io::IoContext>(),
        "/dev/QD4310-1",
        115200,
        8,
        qdriver::io::SerialPortBase::parity::none,
        qdriver::io::SerialPortBase::stop_bits::one
    );

    qdriver::interface::Interface interface(serialPortPtr);

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
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }

    return 0;
}
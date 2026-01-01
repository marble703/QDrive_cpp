#include "interface.hpp"
#include "serial/serial.hpp"

#include <iostream>

int main() {
    auto ioContext_0 = std::make_unique<qdriver::io::IoContext>();
    auto ioContext_1 = std::make_unique<qdriver::io::IoContext>();


    std::shared_ptr<qdriver::io::Serial> serialPortPtr_0 = std::make_shared<qdriver::io::Serial>(
        std::move(ioContext_0),
        "/dev/QD4310-0",
        115200,
        8,
        qdriver::io::SerialPortBase::parity::none,
        qdriver::io::SerialPortBase::stop_bits::one
    );

    qdriver::interface::Interface interface_0(serialPortPtr_0);

    std::shared_ptr<qdriver::io::Serial> serialPortPtr_1 = std::make_shared<qdriver::io::Serial>(
        std::move(ioContext_1),
        "/dev/QD4310-1",
        115200,
        8,
        qdriver::io::SerialPortBase::parity::none,
        qdriver::io::SerialPortBase::stop_bits::one
    );

    qdriver::interface::Interface interface_1(serialPortPtr_1);

    if (interface_0.isPortOpen() && interface_1.isPortOpen()) {
        std::cout << "port opened successfully." << std::endl;
    } else {
        std::cout << "Failed to open port." << std::endl;
        return 0;
    }
    interface_0.enable();
    interface_1.enable();

    while (true) {
        interface_0.ctrlStepAngle(0.01);
        interface_1.ctrlStepAngle(0.01);
        std::this_thread::sleep_for(std::chrono::milliseconds(30));
    }

    return 0;
}
#include "interface.hpp"
#include "serial.hpp"

#include <iostream>

int main() {
    auto ioContext = std::make_unique<qdriver::io::IoContext>();

    std::shared_ptr<qdriver::io::Serial> serialPortPtr = std::make_shared<qdriver::io::Serial>(
        std::move(ioContext),
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
        return 0;
    }
    interface.help();
    sleep(1);
    interface.enable();
    sleep(1);
    interface.ctrlAngle(0);
    sleep(1);
    interface.ctrlAngle(std::numbers::pi_v<float> / 180 * 90);
    sleep(1);
    interface.disable();
    sleep(1);
    return 0;
}
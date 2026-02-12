#include "motor.hpp"

#include <chrono>
#include <iostream>
#include <numbers>
#include <thread>

using qdriver::interface::ioType::SERIAL;

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

    auto interfacePtr = std::make_shared<qdriver::interface::Interface>(serialPortPtr);

    if (interfacePtr->isPortOpen()) {
        std::cout << "port opened successfully." << std::endl;
    } else {
        std::cout << "Failed to open port." << std::endl;
        return 0;
    }

    qdriver::motor::Motor motor(interfacePtr, "qd4310_0");

    motor.enable(SERIAL);
    std::this_thread::sleep_for(std::chrono::seconds(1));
    motor.ctrlAngle(0, SERIAL);
    std::this_thread::sleep_for(std::chrono::seconds(1));
    motor.ctrlAngle(std::numbers::pi_v<float> / 180 * 90, SERIAL);
    std::this_thread::sleep_for(std::chrono::seconds(1));
    motor.disable(SERIAL);
    std::this_thread::sleep_for(std::chrono::seconds(1));

    return 0;
}
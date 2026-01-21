#include "motor.hpp"

#include <iostream>

using qdriver::interface::ioType::CAN;

int main() {
    std::shared_ptr<qdriver::io::Can> canBusPtr = std::make_shared<qdriver::io::Can>("can0");

    auto interfacePtr = std::make_shared<qdriver::interface::Interface>(canBusPtr);

    if (interfacePtr->isPortOpen()) {
        std::cout << "CAN port opened successfully." << std::endl;
    } else {
        std::cout << "Failed to open CAN port." << std::endl;
        return 0;
    }

    qdriver::motor::Motor motor(interfacePtr, "qd4310_0", 0x400, 0x500);

    motor.enable(CAN);
    sleep(1);
    motor.ctrlAngle(0, CAN);
    sleep(1);
    motor.ctrlAngle(std::numbers::pi_v<float> / 180 * 90, CAN);
    sleep(1);
    motor.disable(CAN);
    sleep(1);

    return 0;
}
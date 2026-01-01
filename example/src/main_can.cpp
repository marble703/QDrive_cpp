#include "can/can.hpp"
#include "interface.hpp"

#include <iostream>
#include <memory>

int main() {
    std::shared_ptr<qdriver::io::Can> canBusPtr = std::make_shared<qdriver::io::Can>("can0");

    qdriver::interface::Interface interface(canBusPtr);

    if (interface.isPortOpen()) {
        std::cout << "CAN port opened successfully." << std::endl;
    } else {
        std::cout << "Failed to open CAN port." << std::endl;
        return 0;
    }

    interface.enable(0x400);
    sleep(1);
    interface.ctrlAngle(0, 0x400);
    sleep(1);
    interface.ctrlAngle(std::numbers::pi_v<float> / 180 * 90, 0x400);
    sleep(1);
    interface.disable(0x400);
    sleep(1);

    return 0;
}
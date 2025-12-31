#include "can/can.hpp"
#include "interfacebase.hpp"

#include <iostream>
#include <memory>

int main() {
    std::shared_ptr<qdriver::io::Can> canBusPtr = std::make_shared<qdriver::io::Can>("can0");

    qdriver::interface::InterfaceBase interface(canBusPtr);

    if (interface.isPortOpen()) {
        std::cout << "CAN port opened successfully." << std::endl;
    } else {
        std::cout << "Failed to open CAN port." << std::endl;
        return 0;
    }

    sleep(1);
    if (interface.sendCommand({ .id = 0x400, .ctrlCommand = 0x01, .ctrlValue = 0 })) {
        std::cout << "Sent start command." << std::endl;
    }
    sleep(1);
    interface.sendCommand({ .id = 0x400, .ctrlCommand = 0x05, .ctrlValue = 0 });
    std::cout << "Sent zero angle command." << std::endl;
    sleep(1);
    interface.sendCommand({ .id = 0x400, .ctrlCommand = 0x05, .ctrlValue = 3 });
    std::cout << "Sent set angle to 3 degrees command." << std::endl;
    sleep(1);
    interface.sendCommand({ .id = 0x400, .ctrlCommand = 0x02, .ctrlValue = 0 });
    std::cout << "Sent stop command." << std::endl;
    sleep(1);

    return 0;
}
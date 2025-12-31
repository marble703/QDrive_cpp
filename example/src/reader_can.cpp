#include "base/interfacebase.hpp"
#include "can/can.hpp"

#include <iostream>

int main() {
    std::shared_ptr<qdriver::io::Can> canBusPtr = std::make_shared<qdriver::io::Can>("can0");

    qdriver::interface::InterfaceBase interface(canBusPtr);

    if (interface.isPortOpen()) {
        std::cout << "CAN port opened successfully." << std::endl;
    } else {
        std::cout << "Failed to open CAN port." << std::endl;
        return 0;
    }

    interface.startReaderThread([](std::string& buffer) {
        std::cout << std::hex << buffer << std::flush << std::endl << std::dec;
        return;
    });

    while (true) {
        sleep(1);
    }

    return 0;
}
#include "base/interfacebase.hpp"
#include "can/can.hpp"

#include <iomanip>
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
        auto pos = buffer.find(":");
        if (pos == std::string::npos) {
            return;
        }

        std::string idStr   = buffer.substr(0, pos);
        std::string dataHex = buffer.substr(pos + 1);

        int id = 0;
        try {
            id = std::stoi(idStr);
        } catch (...) {
            return;
        }

        std::size_t dlc = dataHex.size() / 2; // 每两个字符是 1 字节

        // 输出格式参考 candump ：can0  400   [3]  00 00 00
        std::cout << "can0  " << std::uppercase << std::hex << id << "   [" << std::dec << dlc
                  << "]  ";

        for (std::size_t i = 0; i + 1 < dataHex.size(); i += 2) {
            std::string byteStr  = dataHex.substr(i, 2);
            unsigned int byteVal = 0;
            try {
                byteVal = std::stoul(byteStr, nullptr, 16);
            } catch (...) {
                return;
            }

            std::cout << std::setw(2) << std::setfill('0') << std::uppercase << std::hex << byteVal;

            if (i + 2 < dataHex.size()) {
                std::cout << ' ';
            }
        }

        std::cout << std::dec << std::endl;
    });

    while (true) {
        sleep(1);
    }

    return 0;
}
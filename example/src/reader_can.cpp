#include "logger.hpp"
#include "motor.hpp"

int main() {
    auto logger = qdriver::logger::LoggerFactory::createConsoleLogger(
        "qdrive_reader_can",
        spdlog::level::info
    );

    std::shared_ptr<qdriver::io::Can> canBusPtr = std::make_shared<qdriver::io::Can>("can0");

    auto interfacePtr = std::make_shared<qdriver::interface::Interface>(canBusPtr);

    qdriver::motor::Motor motor(interfacePtr, "qd4310_0", 0x400, 0x500);

    if (interfacePtr->isPortOpen()) {
        logger->info("CAN port opened successfully.");
    } else {
        logger->error("Failed to open CAN port.");
        return 0;
    }

    interfacePtr->startReaderThread([logger](std::string& buffer) {
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
        std::ostringstream oss;
        oss << "can0  " << std::uppercase << std::hex << id << "   [" << std::dec << dlc << "]  ";

        for (std::size_t i = 0; i + 1 < dataHex.size(); i += 2) {
            std::string byteStr  = dataHex.substr(i, 2);
            unsigned int byteVal = 0;
            try {
                unsigned long parsed = std::stoul(byteStr, nullptr, 16);
                if (parsed > std::numeric_limits<unsigned int>::max())
                    return;
                byteVal = static_cast<unsigned int>(parsed);
            } catch (...) {
                return;
            }

            oss << std::setw(2) << std::setfill('0') << std::uppercase << std::hex << byteVal;

            if (i + 2 < dataHex.size()) {
                oss << ' ';
            }
        }

        logger->info("{}", oss.str());
    });

    while (true) {
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
        motor.status();
    }

    return 0;
}
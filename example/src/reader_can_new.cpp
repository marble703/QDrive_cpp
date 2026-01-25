#include "logger.hpp"
#include "motor.hpp"

#include <fmt/format.h>

std::string convertStatusToString(qdriver::motor::MotorStatus status) {
    switch (status) {
        case qdriver::motor::MotorStatus::NOP:
            return "NOP";
        case qdriver::motor::MotorStatus::ENABLE:
            return "ENABLE";
        case qdriver::motor::MotorStatus::DISABLE:
            return "DISABLE";
        case qdriver::motor::MotorStatus::CURRENT_CTRL:
            return "CURRENT_CTRL";
        case qdriver::motor::MotorStatus::SPEED_CTRL:
            return "SPEED_CTRL";
        case qdriver::motor::MotorStatus::ANGLE_CTRL:
            return "ANGLE_CTRL";
        case qdriver::motor::MotorStatus::LOW_SPEED_CTRL:
            return "LOW_SPEED_CTRL";
        default:
            return "UNKNOWN";
    }
}

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

    motor.startReaderThread([logger](qdriver::motor::CanMessage& msg) {
        logger->info(
            "Received message: canID: {:#X}, status: {}, current: {}, speed: {}, angle:{}",
            msg.canID,
            convertStatusToString(msg.status),
            msg.current,
            msg.speed,
            msg.angle
        );
    });

    while (true) {
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
        motor.status();
    }

    return 0;
}
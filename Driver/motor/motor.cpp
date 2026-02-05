#include "motor.hpp"

#include <limits>

namespace qdriver::motor {
Motor::Motor(
    std::shared_ptr<qdriver::interface::Interface> interfacePtr,
    const std::string& name,
    uint32_t sendCanID,
    uint32_t receiveCanID,
    std::shared_ptr<qdriver::logger::Logger> logger
):
    name_(name),
    sendCanID_(sendCanID),
    receiveCanID_(receiveCanID),
    logger_(logger) {
    logger_->info("[Motor] Creating motor '{}'", this->name_);
    this->addInterface(interfacePtr);
}

bool Motor::addInterface(std::shared_ptr<qdriver::interface::Interface> interfacePtr) {
    assert(
        static_cast<size_t>(interfacePtr->getIoType()) < interfaces_.size()
    ); // 确保接口类型在数组范围内

    const size_t ioEnum = static_cast<size_t>(interfacePtr->getIoType());
    // 已存在该类型接口
    if (interfaces_[ioEnum] != nullptr) {
        logger_->warn("[Motor] Interface type already exists for motor '{}'", name_);
        return false;
    }

    std::shared_ptr<std::string> ioTypeName = std::make_shared<std::string>();
    interfacePtr->getIoType(ioTypeName);
    logger_->info("[Motor] Added {} interface to motor '{}'", *ioTypeName, name_);

    this->interfaces_[ioEnum] = interfacePtr;
    return true;
}

bool Motor::removeInterface(ioType ioType) {
    assert(static_cast<size_t>(ioType) < interfaces_.size()); // 确保接口类型在数组范围内

    const size_t ioEnum = static_cast<size_t>(ioType);
    // 不存在该类型接口
    if (interfaces_[ioEnum] == nullptr) {
        return false;
    }
    interfaces_[ioEnum] = nullptr;
    return true;
}

std::shared_ptr<qdriver::interface::Interface> Motor::getInterface(const ioType ioType) const {
    return interfaces_[static_cast<size_t>(ioType)];
}

bool Motor::help() {
    if (this->getInterface(ioType::SERIAL)) {
        return this->getInterface(ioType::SERIAL)->help();
    }
    return false;
}

bool Motor::version() {
    if (this->getInterface(ioType::SERIAL)) {
        return this->getInterface(ioType::SERIAL)->version();
    }
    return false;
}

bool Motor::info() {
    if (this->getInterface(ioType::SERIAL)) {
        return this->getInterface(ioType::SERIAL)->info();
    }
    return false;
}

bool Motor::status(ioType ioType) {
    if (ioType == ioType::NONE && this->getInterface(ioType::SERIAL) || ioType == ioType::SERIAL) {
        return this->getInterface(ioType::SERIAL)->status();
    } else if (ioType == ioType::NONE && this->getInterface(ioType::CAN) || ioType == ioType::CAN) {
        return this->getInterface(ioType::CAN)->status(this->sendCanID_);
    }
    return false;
}

bool Motor::enable(ioType ioType) {
    logger_->info("[Motor] Enabling motor '{}'", name_);
    if (ioType == ioType::NONE && this->getInterface(ioType::SERIAL) || ioType == ioType::SERIAL) {
        return this->getInterface(ioType::SERIAL)->enable();
    } else if (ioType == ioType::NONE && this->getInterface(ioType::CAN) || ioType == ioType::CAN) {
        return this->getInterface(ioType::CAN)->enable(this->sendCanID_);
    }
    return false;
}

bool Motor::disable(ioType ioType) {
    logger_->info("[Motor] Disabling motor '{}'", name_);
    if (ioType == ioType::NONE && this->getInterface(ioType::SERIAL) || ioType == ioType::SERIAL) {
        return this->getInterface(ioType::SERIAL)->disable();
    } else if (ioType == ioType::NONE && this->getInterface(ioType::CAN) || ioType == ioType::CAN) {
        return this->getInterface(ioType::CAN)->disable(this->sendCanID_);
    }
    return false;
}

bool Motor::silent() {
    if (this->getInterface(ioType::SERIAL)) {
        return this->getInterface(ioType::SERIAL)->silent();
    }
    return false;
}

bool Motor::reboot() {
    if (this->getInterface(ioType::SERIAL)) {
        return this->getInterface(ioType::SERIAL)->reboot();
    }
    return false;
}

bool Motor::ctrlCurrent(float current, ioType ioType) {
    logger_->debug("[Motor] Motor '{}' controlling current: {} A", name_, current);
    if (ioType == ioType::NONE && this->getInterface(ioType::SERIAL) || ioType == ioType::SERIAL) {
        return this->getInterface(ioType::SERIAL)->ctrlCurrent(current);
    } else if (ioType == ioType::NONE && this->getInterface(ioType::CAN) || ioType == ioType::CAN) {
        return this->getInterface(ioType::CAN)->ctrlCurrent(current, this->sendCanID_);
    }
    return false;
}

bool Motor::ctrlSpeed(float speed, ioType ioType) {
    logger_->debug("[Motor] Motor '{}' controlling speed: {} rpm", name_, speed);
    if (ioType == ioType::NONE && this->getInterface(ioType::SERIAL) || ioType == ioType::SERIAL) {
        return this->getInterface(ioType::SERIAL)->ctrlSpeed(speed);
    } else if (ioType == ioType::NONE && this->getInterface(ioType::CAN) || ioType == ioType::CAN) {
        return this->getInterface(ioType::CAN)->ctrlSpeed(speed, this->sendCanID_);
    }
    return false;
}

bool Motor::ctrlAngle(float angle, ioType ioType) {
    // 考虑环形限制
    if (this->minAngle_ < this->maxAngle_) {
        if (angle < this->minAngle_ || angle > this->maxAngle_) {
            logger_->warn(
                "[Motor] Motor '{}' angle {} out of range [{}, {}]",
                name_,
                angle,
                minAngle_,
                maxAngle_
            );
            return false;
        }
    } else if (this->minAngle_ > this->maxAngle_) {
        if (angle > this->minAngle_ || angle < this->maxAngle_) {
            logger_->warn(
                "[Motor] Motor '{}' angle {} out of range [{}, {}]",
                name_,
                angle,
                minAngle_,
                maxAngle_
            );
            return false;
        }
    }

    logger_->debug("[Motor] Motor '{}' controlling angle: {} rad", name_, angle);
    if (ioType == ioType::NONE && this->getInterface(ioType::SERIAL) || ioType == ioType::SERIAL) {
        return this->getInterface(ioType::SERIAL)->ctrlAngle(angle);
    } else if (ioType == ioType::NONE && this->getInterface(ioType::CAN) || ioType == ioType::CAN) {
        return this->getInterface(ioType::CAN)->ctrlAngle(angle, this->sendCanID_);
    }
    return false;
}

bool Motor::ctrlLowSpeed(float speed, ioType ioType) {
    if (ioType == ioType::NONE && this->getInterface(ioType::SERIAL) || ioType == ioType::SERIAL) {
        return this->getInterface(ioType::SERIAL)->ctrlLowSpeed(speed);
    } else if (ioType == ioType::NONE && this->getInterface(ioType::CAN) || ioType == ioType::CAN) {
        return this->getInterface(ioType::CAN)->ctrlLowSpeed(speed, this->sendCanID_);
    }
    return false;
}

bool Motor::ctrlStepAngle(float angle, ioType ioType) {
    if (ioType == ioType::NONE && this->getInterface(ioType::SERIAL) || ioType == ioType::SERIAL) {
        return this->getInterface(ioType::SERIAL)->ctrlStepAngle(angle);
    } else if (ioType == ioType::NONE && this->getInterface(ioType::CAN) || ioType == ioType::CAN) {
        return this->getInterface(ioType::CAN)->ctrlStepAngle(angle, this->sendCanID_);
    }
    return false;
}

bool Motor::store() {
    if (this->getInterface(ioType::SERIAL)) {
        return this->getInterface(ioType::SERIAL)->store();
    }
    return false;
}

bool Motor::restore() {
    if (this->getInterface(ioType::SERIAL)) {
        return this->getInterface(ioType::SERIAL)->restore();
    }
    return false;
}

bool Motor::configSpeed(float value, PIDtype pidType) {
    if (this->getInterface(ioType::SERIAL)) {
        return this->getInterface(ioType::SERIAL)->configSpeed(value, pidType);
    } else if (this->getInterface(ioType::CAN)) {
        return this->getInterface(ioType::CAN)->configSpeed(value, pidType);
    }
    return false;
}

bool Motor::configAngle(float value, PIDtype pidType) {
    if (this->getInterface(ioType::SERIAL)) {
        return this->getInterface(ioType::SERIAL)->configAngle(value, pidType);
    } else if (this->getInterface(ioType::CAN)) {
        return this->getInterface(ioType::CAN)->configAngle(value, pidType);
    }
    return false;
}

bool Motor::configLimitSpeed(float speed) {
    if (this->getInterface(ioType::SERIAL)) {
        return this->getInterface(ioType::SERIAL)->configLimitSpeed(speed);
    } else if (this->getInterface(ioType::CAN)) {
        return this->getInterface(ioType::CAN)->configLimitSpeed(speed);
    }
    return false;
}

bool Motor::configLimitCurrent(float current) {
    if (this->getInterface(ioType::SERIAL)) {
        return this->getInterface(ioType::SERIAL)->configLimitCurrent(current);
    } else if (this->getInterface(ioType::CAN)) {
        return this->getInterface(ioType::CAN)->configLimitCurrent(current);
    }
    return false;
}

bool Motor::configCanID(uint32_t canID) {
    if (this->getInterface(ioType::SERIAL)) {
        return this->getInterface(ioType::SERIAL)->configCanID(canID);
    } else if (this->getInterface(ioType::CAN)) {
        return this->getInterface(ioType::CAN)->configCanID(canID);
    }
    return false;
}

bool Motor::configBaudRate(unsigned int baudRate) {
    if (this->getInterface(ioType::SERIAL)) {
        return this->getInterface(ioType::SERIAL)->configBaudRate(baudRate);
    }
    return false;
}

bool Motor::getCanID(uint32_t& sendCanID, uint32_t& receiveCanID) const {
    sendCanID    = this->sendCanID_;
    receiveCanID = this->receiveCanID_;
    return true;
}

bool Motor::setCanID(uint32_t sendCanID, uint32_t receiveCanID) {
    if (sendCanID <= qdriver::interface::HIGH_SEND_CAN_ID
        && sendCanID >= qdriver::interface::LOW_SEND_CAN_ID
        && receiveCanID <= qdriver::interface::HIGH_RECV_CAN_ID
        && receiveCanID >= qdriver::interface::LOW_RECV_CAN_ID)
    {
        this->sendCanID_    = sendCanID;
        this->receiveCanID_ = receiveCanID;
        return true;
    }
    return false;
}

bool Motor::getConfig() {
    if (this->getInterface(ioType::SERIAL)) {
        return this->getInterface(ioType::SERIAL)->getConfig();
    }
    return false;
}

bool Motor::startReaderThread(ioType ioType, std::function<void(std::string&)> readerFunction) {
    if (this->getInterface(ioType)) {
        return this->getInterface(ioType)->startReaderThread(readerFunction);
    }
    return false;
}

bool Motor::startReaderThread(std::function<void(CanMessage&)> readerFunction) {
    if (this->getInterface(ioType::CAN)) {
        return this->getInterface(ioType::CAN)
            ->startReaderThread([readerFunction](const std::string& msg) {
                try {
                    size_t colonPos = msg.find(':');
                    if (colonPos == std::string::npos)
                        return;

                    std::string idStr   = msg.substr(0, colonPos);
                    std::string dataStr = msg.substr(colonPos + 1);

                    uint32_t id = 0;
                    try {
                        unsigned long parsed = std::stoul(idStr);
                        if (parsed > std::numeric_limits<uint32_t>::max())
                            return;
                        id = static_cast<uint32_t>(parsed);
                    } catch (...) {
                        return;
                    }

                    std::vector<uint8_t> data;
                    for (size_t i = 0; i + 1 < dataStr.size(); i += 2) {
                        std::string byteString = dataStr.substr(i, 2);
                        try {
                            unsigned long val = std::stoul(byteString, nullptr, 16);
                            if (val > std::numeric_limits<uint8_t>::max())
                                return;
                            data.push_back(static_cast<uint8_t>(val));
                        } catch (...) {
                            return;
                        }
                    }

                    if (data.size() < 8)
                        return;

                    CanMessage canMsg;
                    canMsg.canID   = id;
                    canMsg.status  = static_cast<MotorStatus>(data[0]);
                    uint16_t currentRaw = static_cast<uint16_t>(
                        static_cast<uint16_t>(data[2])
                        | (static_cast<uint16_t>(data[3]) << 8)
                    );
                    uint16_t speedRaw = static_cast<uint16_t>(
                        static_cast<uint16_t>(data[4])
                        | (static_cast<uint16_t>(data[5]) << 8)
                    );
                    uint16_t angleRaw = static_cast<uint16_t>(
                        static_cast<uint16_t>(data[6])
                        | (static_cast<uint16_t>(data[7]) << 8)
                    );

                    canMsg.current = qdriver::interface::ctrlValueToCurrent(
                        static_cast<int16_t>(currentRaw)
                    );
                    canMsg.speed = qdriver::interface::ctrlValueToSpeed(
                        static_cast<int16_t>(speedRaw)
                    );
                    canMsg.angle = qdriver::interface::ctrlValueToAngle(angleRaw);

                    readerFunction(canMsg);
                } catch (...) {}
            });
    }
    return false;
}

bool Motor::ReleaseReaderThread(ioType ioType) {
    if (this->getInterface(ioType)) {
        return this->getInterface(ioType)->ReleaseReaderThread();
    }
    return false;
}

} // namespace qdriver::motor
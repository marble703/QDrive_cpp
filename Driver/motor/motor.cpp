#include "motor.hpp"

namespace qdriver::motor {
Motor::Motor(
    std::shared_ptr<qdriver::interface::Interface> interfacePtr,
    const std::string& name,
    size_t sendCanID,
    size_t receiveCanID
):
    name_(name),
    sendCanID_(sendCanID),
    receiveCanID_(receiveCanID) {
    this->addInterface(interfacePtr);
}

bool Motor::addInterface(std::shared_ptr<qdriver::interface::Interface> interfacePtr) {
    assert(
        static_cast<size_t>(interfacePtr->getIoType()) < interfaces_.size()
    ); // 确保接口类型在数组范围内

    const size_t ioEnum = static_cast<size_t>(interfacePtr->getIoType());
    // 已存在该类型接口
    if (interfaces_[ioEnum] != nullptr) {
        return false;
    }
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
    if (ioType == ioType::NONE && this->getInterface(ioType::SERIAL)) {
        return this->getInterface(ioType::SERIAL)->status();
    } else if (ioType == ioType::NONE && this->getInterface(ioType::CAN)) {
        return this->getInterface(ioType::CAN)->status(this->sendCanID_);
    }
    return false;
}

bool Motor::enable(ioType ioType) {
    if (ioType == ioType::NONE && this->getInterface(ioType::SERIAL)) {
        return this->getInterface(ioType::SERIAL)->enable();
    } else if (ioType == ioType::NONE && this->getInterface(ioType::CAN)) {
        return this->getInterface(ioType::CAN)->enable(this->sendCanID_);
    }
    return false;
}

bool Motor::disable(ioType ioType) {
    if (ioType == ioType::NONE && this->getInterface(ioType::SERIAL)) {
        return this->getInterface(ioType::SERIAL)->disable();
    } else if (ioType == ioType::NONE && this->getInterface(ioType::CAN)) {
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
    if (ioType == ioType::NONE && this->getInterface(ioType::SERIAL)) {
        return this->getInterface(ioType::SERIAL)->ctrlCurrent(current);
    } else if (ioType == ioType::NONE && this->getInterface(ioType::CAN)) {
        return this->getInterface(ioType::CAN)->ctrlCurrent(current, this->sendCanID_);
    }
    return false;
}

bool Motor::ctrlSpeed(float speed, ioType ioType) {
    if (ioType == ioType::NONE && this->getInterface(ioType::SERIAL)) {
        return this->getInterface(ioType::SERIAL)->ctrlSpeed(speed);
    } else if (ioType == ioType::NONE && this->getInterface(ioType::CAN)) {
        return this->getInterface(ioType::CAN)->ctrlSpeed(speed, this->sendCanID_);
    }
    return false;
}

bool Motor::ctrlAngle(float angle, ioType ioType) {
    if (angle < this->minAngle_ || angle > this->maxAngle_) {
        return false;
    }

    if (ioType == ioType::NONE && this->getInterface(ioType::SERIAL)) {
        return this->getInterface(ioType::SERIAL)->ctrlAngle(angle);
    } else if (ioType == ioType::NONE && this->getInterface(ioType::CAN)) {
        return this->getInterface(ioType::CAN)->ctrlAngle(angle, this->sendCanID_);
    }
    return false;
}

bool Motor::ctrlLowSpeed(float speed, ioType ioType) {
    if (ioType == ioType::NONE && this->getInterface(ioType::SERIAL)) {
        return this->getInterface(ioType::SERIAL)->ctrlLowSpeed(speed);
    } else if (ioType == ioType::NONE && this->getInterface(ioType::CAN)) {
        return this->getInterface(ioType::CAN)->ctrlLowSpeed(speed, this->sendCanID_);
    }
    return false;
}

bool Motor::ctrlStepAngle(float angle, ioType ioType) {
    if (ioType == ioType::NONE && this->getInterface(ioType::SERIAL)) {
        return this->getInterface(ioType::SERIAL)->ctrlStepAngle(angle);
    } else if (ioType == ioType::NONE && this->getInterface(ioType::CAN)) {
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

bool Motor::getCanID(size_t& sendCanID, size_t& receiveCanID) const {
    sendCanID    = this->sendCanID_;
    receiveCanID = this->receiveCanID_;
    return true;
}

bool Motor::setCanID(size_t sendCanID, size_t receiveCanID) {
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

bool Motor::startReaderThread(ioType ioType, std::function<void(std::string&)> readerFunction) {
    if (this->getInterface(ioType)) {
        return this->getInterface(ioType)->startReaderThread(readerFunction);
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
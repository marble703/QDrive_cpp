#include "motor.hpp"

namespace qdriver::motor {
motor::motor(
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

bool motor::addInterface(std::shared_ptr<qdriver::interface::Interface> interfacePtr) {
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

bool motor::removeInterface(ioType ioType) {
    assert(static_cast<size_t>(ioType) < interfaces_.size()); // 确保接口类型在数组范围内

    const size_t ioEnum = static_cast<size_t>(ioType);
    // 不存在该类型接口
    if (interfaces_[ioEnum] == nullptr) {
        return false;
    }
    interfaces_[ioEnum] = nullptr;
    return true;
}

bool motor::setCanID(size_t sendCanID, size_t receiveCanID) {
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
} // namespace qdriver::motor
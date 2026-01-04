#include "interface.hpp"

namespace qdriver::motor {

using qdriver::interface::ioType;

class motor {
public:
    motor(
        std::shared_ptr<qdriver::interface::Interface> interfacePtr,
        const std::string& name = "defaultmotor",
        size_t sendCanID        = 0x400,
        size_t receiveCanID     = 0x500
    );

    bool addInterface(std::shared_ptr<qdriver::interface::Interface> interfacePtr);

    bool removeInterface(const ioType);

    bool setCanID(size_t sendCanID, size_t receiveCanID);
    

private:
    std::string name_;
    size_t sendCanID_;
    size_t receiveCanID_;

    std::array<std::shared_ptr<qdriver::interface::Interface>, 2> interfaces_;
};
} // namespace qdriver::motor
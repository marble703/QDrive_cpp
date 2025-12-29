#include "interfacebase.hpp"
#include <memory>
#include <string>

namespace qdriver::interface {

class Interface: InterfaceBase {
public:
    Interface(std::shared_ptr<qdriver::io::Serial> serialPort);
    Interface(std::shared_ptr<qdriver::io::Can> canPort) = delete; // 还没写 CAN

    ~Interface();
};

} // namespace qdriver::interface

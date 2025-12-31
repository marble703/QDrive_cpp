#include "interfacebase.hpp"

namespace qdriver::interface {

class Interface: InterfaceBase {
public:
    Interface(std::shared_ptr<qdriver::io::Serial> serialPort);
    Interface(std::shared_ptr<qdriver::io::Can> canPort);

    ~Interface();
};

} // namespace qdriver::interface

#include "interface.hpp"

namespace qdriver::interface {
Interface::Interface(std::shared_ptr<qdriver::io::Serial> serialPort): InterfaceBase(serialPort) {}
Interface::Interface(std::shared_ptr<qdriver::io::Can> canPort): InterfaceBase(canPort) {}
Interface::~Interface() {}

} // namespace qdriver::interface
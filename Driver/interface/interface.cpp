#include "interface.hpp"

namespace qdriver::interface {
Interface::Interface(std::shared_ptr<qdriver::io::Serial> serialPort): InterfaceBase(serialPort) {}
Interface::~Interface() {}

} // namespace qdriver::interface
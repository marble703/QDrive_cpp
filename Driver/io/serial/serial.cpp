#include "serial.hpp"
#include <string>

namespace qdriver::io {

Serial::Serial(
    std::unique_ptr<IoContext> ioContext,
    const std::filesystem::path& devicePath,
    SerialPortConfig config,
    std::string portName
):
    ioContext_(std::move(ioContext)),
    serialPort_(*ioContext, devicePath.string()),
    portName_(portName.empty() ? devicePath.string() : portName) {
    serialPort_.set_option(SerialPortBase::baud_rate(config.baud_rate));
    serialPort_.set_option(SerialPortBase::character_size(config.data_bits));
    serialPort_.set_option(SerialPortBase::parity(config.parity));
    serialPort_.set_option(SerialPortBase::stop_bits(config.stop_bits));
}

Serial::Serial(
    std::unique_ptr<IoContext> ioContext,
    const std::filesystem::path& devicePath,
    unsigned int baud_rate,
    unsigned int data_bits,
    SerialPortBase::parity::type parity,
    SerialPortBase::stop_bits::type stop_bits,
    std::string portName
):
    Serial(
        std::move(ioContext),
        devicePath,
        SerialPortConfig { .baud_rate = baud_rate,
                           .data_bits = data_bits,
                           .parity    = parity,
                           .stop_bits = stop_bits },
        portName
    ) {}

Serial::~Serial() {
    if (serialPort_.is_open()) {
        serialPort_.close();
    }
}

bool Serial::isOpen() const {
    return serialPort_.is_open();
}

std::string Serial::getPortName() const {
    return this->portName_;
}

bool Serial::read(std::string& buffer, std::size_t size, std::shared_ptr<std::size_t> bytesRead) {
    boost::system::error_code ec;
    std::size_t bytesReadLocal = this->serialPort_.read_some(boost::asio::buffer(buffer, size), ec);

    if (bytesRead) {
        *bytesRead = bytesReadLocal;
    }

    return !ec;
}

bool Serial::write(const std::string& data, std::size_t size) {
    boost::system::error_code ec;
    std::size_t bytesWritten = this->serialPort_.write_some(boost::asio::buffer(data, size), ec);
    return !ec && bytesWritten == size;
}

} // namespace qdriver::io

#include "serial.hpp"

namespace qdriver::io {

Serial::Serial(
    IOContextPtrSelector ioContext,
    const std::filesystem::path& devicePath,
    SerialPortConfig config,
    std::string portName,
    std::shared_ptr<qdriver::logger::Logger> logger
):
    ioContext_(std::move(ioContext)),
    serialPort_(*ioContext.get(), devicePath.string()),
    portName_(portName.empty() ? devicePath.string() : portName),
    logger_(logger ? logger : qdriver::logger::LoggerFactory::getDefaultLogger()) {
    logger_->info("[Serial] Initializing serial port: {}", portName_);
    serialPort_.set_option(SerialPortBase::baud_rate(config.baud_rate));
    serialPort_.set_option(SerialPortBase::character_size(config.data_bits));
    serialPort_.set_option(SerialPortBase::parity(config.parity));
    serialPort_.set_option(SerialPortBase::stop_bits(config.stop_bits));
    logger_->info("[Serial] Serial port {} configured: baud={}, data_bits={}", 
                  portName_, config.baud_rate, config.data_bits);
}

Serial::Serial(
    IOContextPtrSelector ioContext,
    const std::filesystem::path& devicePath,
    unsigned int baud_rate,
    unsigned int data_bits,
    SerialPortBase::parity::type parity,
    SerialPortBase::stop_bits::type stop_bits,
    std::string portName,
    std::shared_ptr<qdriver::logger::Logger> logger
):
    Serial(
        std::move(ioContext),
        devicePath,
        SerialPortConfig { .baud_rate = baud_rate,
                           .data_bits = data_bits,
                           .parity    = parity,
                           .stop_bits = stop_bits },
        portName,
        logger
    ) {}

Serial::~Serial() {
    close();
}

bool Serial::isOpen() const {
    return serialPort_.is_open();
}

void Serial::close() noexcept {
    if (!serialPort_.is_open()) {
        return;
    }

    try {
        logger_->info("[Serial] Closing serial port: {}", portName_);
        boost::system::error_code ec;
        serialPort_.cancel(ec);
        serialPort_.close(ec);
        if (ec) {
            logger_->warn("[Serial] Close error on {}: {}", portName_, ec.message());
        }
    } catch (...) {
        // best-effort shutdown
    }
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

    if (ec) {
        logger_->error("[Serial] Read error on {}: {}", portName_, ec.message());
    } else {
        logger_->trace("[Serial] Read {} bytes from {}", bytesReadLocal, portName_);
    }

    return !ec;
}

bool Serial::write(const std::string& data, std::size_t size) {
    boost::system::error_code ec;
    std::size_t bytesWritten = this->serialPort_.write_some(boost::asio::buffer(data, size), ec);
    
    if (ec) {
        logger_->error("[Serial] Write error on {}: {}", portName_, ec.message());
    } else if (bytesWritten != size) {
        logger_->warn("[Serial] Partial write on {}: {} of {} bytes", portName_, bytesWritten, size);
    } else {
        logger_->trace("[Serial] Wrote {} bytes to {}", bytesWritten, portName_);
    }
    
    return !ec && bytesWritten == size;
}

} // namespace qdriver::io

#pragma once

#include <boost/asio/io_context.hpp>
#include <boost/asio/serial_port.hpp>

#include <filesystem>

#include "logger.hpp"

namespace qdriver::io {

using SerialPort     = boost::asio::serial_port;
using SerialPortBase = boost::asio::serial_port_base;
using IoContext      = boost::asio::io_context;

struct SerialPortConfig {
    unsigned int baud_rate                    = 115200;
    unsigned int data_bits                    = 8;
    SerialPortBase::parity::type parity       = SerialPortBase::parity::none;
    SerialPortBase::stop_bits::type stop_bits = SerialPortBase::stop_bits::one;
};

class Serial {
public:
    Serial(
        IoContext& ioContext,
        const std::filesystem::path& devicePath = "/dev/ttyACM0",
        SerialPortConfig config                 = SerialPortConfig {},
        std::string portName                    = std::string(),
        std::shared_ptr<qdriver::logger::Logger> logger =
            std::make_shared<qdriver::logger::Logger>()
    );

    Serial(
        IoContext& ioContext,
        const std::filesystem::path& devicePath   = "/dev/ttyACM0",
        unsigned int baud_rate                    = 115200,
        unsigned int data_bits                    = 8,
        SerialPortBase::parity::type parity       = SerialPortBase::parity::none,
        SerialPortBase::stop_bits::type stop_bits = SerialPortBase::stop_bits::one,
        std::string portName                      = std::string(),
        std::shared_ptr<qdriver::logger::Logger> logger =
            std::make_shared<qdriver::logger::Logger>()
    );

    ~Serial();

    Serial(const Serial&)            = delete;
    Serial& operator=(const Serial&) = delete;

    bool isOpen() const;

    void close() noexcept;

    std::string getPortName() const;

    bool
    read(std::string& buffer, std::size_t size, std::shared_ptr<std::size_t> bytesRead = nullptr);

    bool write(const std::string& data, std::size_t size);

protected:
    const std::string portName_;
    std::shared_ptr<qdriver::logger::Logger> logger_;

private:
    SerialPort serialPort_;
};
} // namespace qdriver::io
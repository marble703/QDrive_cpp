#pragma once

#include <boost/asio/serial_port.hpp>
#include <filesystem>
#include <string>

namespace qdriver::io {

using SerialPort     = boost::asio::serial_port;
using SerialPortBase = boost::asio::serial_port_base;
using IoContext      = boost::asio::io_context;

// 串口配置结构体
struct SerialPortConfig {
    unsigned int baud_rate                    = 115200;
    unsigned int data_bits                    = 8;
    SerialPortBase::parity::type parity       = SerialPortBase::parity::none;
    SerialPortBase::stop_bits::type stop_bits = SerialPortBase::stop_bits::one;
};

class Serial {
public:
    Serial(
        std::unique_ptr<IoContext> ioContext,
        const std::filesystem::path& devicePath = "/dev/ttyACM0",
        SerialPortConfig config                 = SerialPortConfig {},
        std::string portName                    = std::string()
    );

    Serial(
        std::unique_ptr<IoContext> ioContext,
        const std::filesystem::path& devicePath   = "/dev/ttyACM0",
        unsigned int baud_rate                    = 115200,
        unsigned int data_bits                    = 8,
        SerialPortBase::parity::type parity       = SerialPortBase::parity::none,
        SerialPortBase::stop_bits::type stop_bits = SerialPortBase::stop_bits::one,
        std::string portName                      = std::string()
    );

    ~Serial();

    // 串口资源不可拷贝
    Serial(const Serial&)            = delete;
    Serial& operator=(const Serial&) = delete;

    bool isOpen() const;

    std::string getPortName() const;

    // size 作为最大读取长度传入
    bool read(std::string& buffer, std::size_t size, std::shared_ptr<std::size_t> bytesRead = nullptr);

    bool write(const std::string& data, std::size_t size);

protected:
    const std::string portName_;

private:
    SerialPort serialPort_;
    std::unique_ptr<IoContext> ioContext_;
};
} // namespace qdriver::io
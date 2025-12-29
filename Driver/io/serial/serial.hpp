#pragma once

#include <boost/asio/io_context.hpp>
#include <boost/asio/serial_port.hpp>
#include <filesystem>

namespace qdriver::io {

using SerialPort     = boost::asio::serial_port;
using SerialPortBase = boost::asio::serial_port_base;
using IoContext      = boost::asio::io_context;

// 兼容 shared_ptr 和 unique_ptr 的 IOContext 指针
struct IOContextPtrSelector {
public:
    bool useShared = false;

    IOContextPtrSelector(std::shared_ptr<IoContext> sharedPtr):
        shared(sharedPtr),
        unique(nullptr),
        useShared(true) {}
    IOContextPtrSelector(std::unique_ptr<IoContext> uniquePtr):
        shared(nullptr),
        unique(std::move(uniquePtr)),
        useShared(false) {}

    IoContext* get() {
        if (useShared) {
            return shared.get();
        } else {
            return unique.get();
        }
    }

private:
    std::shared_ptr<boost::asio::io_context> shared = nullptr;
    std::unique_ptr<boost::asio::io_context> unique = nullptr;
};
/*
template<typename T, template<typename> class SmartPtr>
struct PtrSelector {
    PtrSelector(SmartPtr<T> ptr): ptr(std::move(ptr)) {}
    SmartPtr<T> ptr;
};
*/

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
        IOContextPtrSelector ioContext,
        const std::filesystem::path& devicePath = "/dev/ttyACM0",
        SerialPortConfig config                 = SerialPortConfig {},
        std::string portName                    = std::string()
    );

    Serial(
        IOContextPtrSelector ioContext,
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
    bool
    read(std::string& buffer, std::size_t size, std::shared_ptr<std::size_t> bytesRead = nullptr);

    bool write(const std::string& data, std::size_t size);

protected:
    const std::string portName_;

private:
    SerialPort serialPort_;
    IOContextPtrSelector ioContext_;
};
} // namespace qdriver::io
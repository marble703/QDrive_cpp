#include "can.hpp"

#include <algorithm>
#include <cctype>
#include <cstring>
#include <iomanip>
#include <optional>
#include <sstream>
#include <stdexcept>

#if defined(__linux__)
    #include <cerrno>
    #include <linux/can.h>
    #include <net/if.h>
    #include <sys/ioctl.h>
    #include <sys/select.h>
    #include <sys/socket.h>
    #include <unistd.h>
#elif defined(_WIN32)
    #ifndef NOMINMAX
        #define NOMINMAX
    #endif
    #include <windows.h>
#endif

namespace qdriver::io {

#if defined(_WIN32)
namespace {

    constexpr DWORD kReadTimeoutMs  = 100;
    constexpr DWORD kWriteTimeoutMs = 100;

    std::optional<std::string> slcanBitrateCommand(unsigned int bitrate) {
        switch (bitrate) {
            case 10000:
                return "S0\r";
            case 20000:
                return "S1\r";
            case 50000:
                return "S2\r";
            case 100000:
                return "S3\r";
            case 125000:
                return "S4\r";
            case 250000:
                return "S5\r";
            case 500000:
                return "S6\r";
            case 750000:
                return "S7\r";
            case 1000000:
                return "S8\r";
            default:
                return std::nullopt;
        }
    }

    bool isHexChar(char c) {
        return std::isxdigit(static_cast<unsigned char>(c)) != 0;
    }

    bool parseHexByte(const std::string& s, std::size_t pos, uint8_t& out) {
        if (pos + 1 >= s.size() || !isHexChar(s[pos]) || !isHexChar(s[pos + 1])) {
            return false;
        }
        unsigned int value = 0;
        std::stringstream ss;
        ss << std::hex << s.substr(pos, 2);
        ss >> value;
        if (ss.fail()) {
            return false;
        }
        out = static_cast<uint8_t>(value);
        return true;
    }

    bool parseHex3(const std::string& s, std::size_t pos, uint32_t& out) {
        if (pos + 2 >= s.size() || !isHexChar(s[pos]) || !isHexChar(s[pos + 1])
            || !isHexChar(s[pos + 2]))
        {
            return false;
        }
        unsigned int value = 0;
        std::stringstream ss;
        ss << std::hex << s.substr(pos, 3);
        ss >> value;
        if (ss.fail()) {
            return false;
        }
        out = value;
        return true;
    }

} // namespace
#endif

Can::Can(const std::string& ifname, std::shared_ptr<qdriver::logger::Logger> logger):
    ifname_(ifname),
    logger_(logger ? logger : qdriver::logger::LoggerFactory::getDefaultLogger()) {
    logger_->info("[CAN] Initializing CAN interface: {}", ifname_);

#if defined(__linux__)
    // 创建 Socket
    sock_ = socket(PF_CAN, SOCK_RAW, CAN_RAW);
    if (sock_ < 0) {
        logger_->error("[CAN] Failed to create CAN socket for {}", ifname_);
        throw std::runtime_error("Failed to create CAN socket");
    }

    ifreq ifr;
    std::memset(&ifr, 0, sizeof(ifr));
    std::strncpy(ifr.ifr_name, ifname.c_str(), IFNAMSIZ - 1);

    // 获取接口索引
    if (ioctl(sock_, SIOCGIFINDEX, &ifr) < 0) {
        logger_->error("[CAN] Failed to get interface index for {}", ifname_);
        close(sock_);
        throw std::runtime_error("Failed to get interface index for " + ifname);
    }

    sockaddr_can addr;
    std::memset(&addr, 0, sizeof(addr));
    addr.can_family  = AF_CAN;
    addr.can_ifindex = ifr.ifr_ifindex;

    // 绑定套接字
    if (bind(sock_, (sockaddr*)&addr, sizeof(addr)) < 0) {
        logger_->error("[CAN] Failed to bind CAN socket to {}", ifname_);
        close(sock_);
        throw std::runtime_error("Failed to bind CAN socket to " + ifname);
    }

    logger_->info("[CAN] CAN interface {} initialized successfully", ifname_);
#elif defined(_WIN32)
    // Windows: SLCAN over serial
    // 支持形式:
    // 1) COM3                -> 默认 500000 bps
    // 2) COM3@500000         -> 指定 CAN 波特率
    // 3) slcan:COM3@500000   -> 显式前缀
    std::string endpoint = ifname_;
    if (endpoint.rfind("slcan:", 0) == 0) {
        endpoint = endpoint.substr(6);
    }

    unsigned int bitrate = 500000;
    if (auto atPos = endpoint.find('@'); atPos != std::string::npos) {
        std::string bitrateStr = endpoint.substr(atPos + 1);
        endpoint               = endpoint.substr(0, atPos);
        if (bitrateStr.empty()) {
            throw std::runtime_error("Invalid CAN endpoint: missing bitrate after '@'");
        }
        bitrate = static_cast<unsigned int>(std::stoul(bitrateStr));
    }

    auto bitrateCmd = slcanBitrateCommand(bitrate);
    if (!bitrateCmd.has_value()) {
        throw std::runtime_error("Unsupported CAN bitrate for SLCAN backend");
    }

    std::string comPath = endpoint;
    if (comPath.rfind("\\\\.\\\\", 0) != 0) {
        comPath = "\\\\.\\\\" + comPath;
    }

    HANDLE h = CreateFileA(
        comPath.c_str(),
        GENERIC_READ | GENERIC_WRITE,
        0,
        nullptr,
        OPEN_EXISTING,
        0,
        nullptr
    );

    if (h == INVALID_HANDLE_VALUE) {
        logger_->error("[CAN] Failed to open SLCAN serial endpoint: {}", comPath);
        throw std::runtime_error("Failed to open SLCAN serial endpoint");
    }

    DCB dcb {};
    dcb.DCBlength = sizeof(DCB);
    if (!GetCommState(h, &dcb)) {
        CloseHandle(h);
        throw std::runtime_error("GetCommState failed for SLCAN serial endpoint");
    }

    dcb.BaudRate = CBR_115200;
    dcb.ByteSize = 8;
    dcb.Parity   = NOPARITY;
    dcb.StopBits = ONESTOPBIT;
    dcb.fBinary  = TRUE;

    if (!SetCommState(h, &dcb)) {
        CloseHandle(h);
        throw std::runtime_error("SetCommState failed for SLCAN serial endpoint");
    }

    COMMTIMEOUTS timeouts {};
    timeouts.ReadIntervalTimeout         = kReadTimeoutMs;
    timeouts.ReadTotalTimeoutConstant    = kReadTimeoutMs;
    timeouts.ReadTotalTimeoutMultiplier  = 0;
    timeouts.WriteTotalTimeoutConstant   = kWriteTimeoutMs;
    timeouts.WriteTotalTimeoutMultiplier = 0;

    if (!SetCommTimeouts(h, &timeouts)) {
        CloseHandle(h);
        throw std::runtime_error("SetCommTimeouts failed for SLCAN serial endpoint");
    }

    PurgeComm(h, PURGE_RXCLEAR | PURGE_TXCLEAR);

    auto writeCommand = [&](const std::string& cmd) {
        DWORD written = 0;
        if (!WriteFile(h, cmd.data(), static_cast<DWORD>(cmd.size()), &written, nullptr)
            || written != cmd.size())
        {
            CloseHandle(h);
            throw std::runtime_error("Failed writing SLCAN command");
        }
    };

    // 先关闭，避免适配器处于未知状态
    writeCommand("C\r");
    writeCommand(*bitrateCmd);
    writeCommand("O\r");

    serialHandle_ = h;
    rxBuffer_.clear();
    logger_->info(
        "[CAN] Windows SLCAN backend initialized: endpoint={}, bitrate={}",
        endpoint,
        bitrate
    );
#else
    logger_->error("[CAN] Unsupported platform for CAN interface: {}", ifname_);
    throw std::runtime_error("Unsupported platform for CAN interface");
#endif
}

Can::~Can() {
#if defined(__linux__)
    if (sock_ >= 0) {
        logger_->info("[CAN] Closing CAN interface: {}", ifname_);
        close(sock_);
    }
#elif defined(_WIN32)
    if (serialHandle_) {
        HANDLE h              = static_cast<HANDLE>(serialHandle_);
        DWORD written         = 0;
        const char closeCmd[] = "C\r";
        WriteFile(h, closeCmd, static_cast<DWORD>(sizeof(closeCmd) - 1), &written, nullptr);
        CloseHandle(h);
        serialHandle_ = nullptr;
    }
#endif
}

bool Can::sendFrame(const std::vector<uint8_t>& data, unsigned int id) {
#if defined(__linux__)
    if (sock_ < 0) {
        logger_->error("[CAN] Cannot send frame: socket not open");
        return false;
    }
    if (data.size() > 8) {
        logger_->error("[CAN] CAN frame data size exceeds 8 bytes: {}", data.size());
        throw std::runtime_error("CAN frame data size exceeds 8 bytes");
    }
    if (id > 0x7FF) {
        logger_->error("[CAN] CAN frame ID exceeds 11 bits: 0x{:X}", id);
        throw std::runtime_error("CAN frame ID exceeds 11 bits");
    }

    can_frame frame;
    std::memset(&frame, 0, sizeof(frame));

    frame.can_id = id;

    if (data.size() > 8) {
        frame.can_dlc = 8;
    } else {
        frame.can_dlc = static_cast<__u8>(data.size());
    }

    std::copy(data.begin(), data.begin() + frame.can_dlc, frame.data);

    bool result = write(sock_, &frame, sizeof(can_frame)) > 0;
    if (result) {
        logger_->trace("[CAN] Sent frame: ID=0x{:X}, DLC={}", id, frame.can_dlc);
    } else {
        logger_->error("[CAN] Failed to send frame: ID=0x{:X}", id);
    }
    return result;
#elif defined(_WIN32)
    if (!serialHandle_) {
        logger_->error("[CAN] Cannot send frame: serial backend not open");
        return false;
    }
    if (data.size() > 8) {
        logger_->error("[CAN] CAN frame data size exceeds 8 bytes: {}", data.size());
        throw std::runtime_error("CAN frame data size exceeds 8 bytes");
    }
    if (id > 0x7FF) {
        logger_->error("[CAN] CAN frame ID exceeds 11 bits: 0x{:X}", id);
        throw std::runtime_error("CAN frame ID exceeds 11 bits");
    }

    std::ostringstream oss;
    oss << 't' << std::uppercase << std::hex << std::setw(3) << std::setfill('0') << id
        << std::uppercase << std::hex << data.size();
    for (uint8_t b: data) {
        oss << std::uppercase << std::hex << std::setw(2) << std::setfill('0')
            << static_cast<unsigned int>(b);
    }
    oss << '\r';
    const std::string frameCmd = oss.str();

    DWORD written = 0;
    HANDLE h      = static_cast<HANDLE>(serialHandle_);
    bool ok = WriteFile(h, frameCmd.data(), static_cast<DWORD>(frameCmd.size()), &written, nullptr)
        && written == frameCmd.size();

    if (!ok) {
        logger_->error("[CAN] Failed to send SLCAN frame: ID=0x{:X}", id);
        return false;
    }
    logger_->trace("[CAN] Sent frame(SLCAN): ID=0x{:X}, DLC={}", id, data.size());
    return true;
#else
    (void)data;
    (void)id;
    logger_->error("[CAN] sendFrame is not supported on this platform");
    return false;
#endif
}

bool Can::receiveFrame(std::vector<uint8_t>& data, std::shared_ptr<uint32_t> id) {
#if defined(__linux__)
    if (sock_ < 0) {
        logger_->error("[CAN] Cannot receive frame: socket not open");
        return false;
    }

    // 使用带超时的 select 避免在 read 上无限阻塞
    fd_set rfds;
    FD_ZERO(&rfds);
    FD_SET(sock_, &rfds);

    // 100ms 超时
    timeval tv;
    tv.tv_sec  = 0;
    tv.tv_usec = 100000; // 100 ms

    int ret = select(sock_ + 1, &rfds, nullptr, nullptr, &tv);

    // 超时或被信号中断，返回 false
    if (ret <= 0) {
        if (ret < 0) {
            logger_->trace("[CAN] Select interrupted or error");
        }
        return false;
    }

    // 文件描述符不可读
    if (!FD_ISSET(sock_, &rfds)) {
        return false;
    }

    can_frame frame;
    ssize_t nbytes = read(sock_, &frame, sizeof(can_frame));

    // 读取错误
    if (nbytes < 0) {
        logger_->error("[CAN] Read error: {}", strerror(errno));
        return false;
    }
    // 帧不完整

    if (nbytes < static_cast<ssize_t>(sizeof(can_frame))) {
        logger_->warn("[CAN] Incomplete frame received: {} bytes", nbytes);
        return false;
    }

    data.assign(frame.data, frame.data + static_cast<std::size_t>(frame.can_dlc));
    if (id)
        *id = frame.can_id;

    logger_->trace("[CAN] Received frame: ID=0x{:X}, DLC={}", frame.can_id, frame.can_dlc);
    return true;
#elif defined(_WIN32)
    if (!serialHandle_) {
        logger_->error("[CAN] Cannot receive frame: serial backend not open");
        return false;
    }

    HANDLE h = static_cast<HANDLE>(serialHandle_);

    auto tryParseFromBuffer = [&]() -> bool {
        while (true) {
            std::size_t endPos = rxBuffer_.find('\r');
            if (endPos == std::string::npos) {
                return false;
            }

            std::string line = rxBuffer_.substr(0, endPos);
            rxBuffer_.erase(0, endPos + 1);

            if (line.empty()) {
                continue;
            }

            // 仅处理标准帧: tIIIldd...
            if (line[0] != 't') {
                continue;
            }
            if (line.size() < 5) {
                continue;
            }

            uint32_t parsedId = 0;
            if (!parseHex3(line, 1, parsedId)) {
                continue;
            }

            char dlcChar = line[4];
            if (!isHexChar(dlcChar)) {
                continue;
            }

            unsigned int dlc = 0;
            {
                std::stringstream ss;
                ss << std::hex << dlcChar;
                ss >> dlc;
                if (ss.fail() || dlc > 8) {
                    continue;
                }
            }

            if (line.size() != 5 + dlc * 2) {
                continue;
            }

            std::vector<uint8_t> parsedData;
            parsedData.reserve(dlc);

            bool ok = true;
            for (unsigned int i = 0; i < dlc; ++i) {
                uint8_t b = 0;
                if (!parseHexByte(line, 5 + i * 2, b)) {
                    ok = false;
                    break;
                }
                parsedData.push_back(b);
            }

            if (!ok) {
                continue;
            }

            data = std::move(parsedData);
            if (id) {
                *id = parsedId;
            }
            logger_->trace("[CAN] Received frame(SLCAN): ID=0x{:X}, DLC={}", parsedId, dlc);
            return true;
        }
    };

    if (tryParseFromBuffer()) {
        return true;
    }

    char readBuf[256] {};
    DWORD bytesRead = 0;

    bool ok = ReadFile(h, readBuf, static_cast<DWORD>(sizeof(readBuf)), &bytesRead, nullptr);
    if (!ok) {
        logger_->error("[CAN] Read error on SLCAN backend");
        return false;
    }

    if (bytesRead == 0) {
        // 超时无数据
        return false;
    }

    rxBuffer_.append(readBuf, readBuf + bytesRead);
    return tryParseFromBuffer();
#else
    (void)data;
    (void)id;
    logger_->error("[CAN] receiveFrame is not supported on this platform");
    return false;
#endif
}

bool Can::isOpen() const {
#if defined(_WIN32)
    return serialHandle_ != nullptr;
#else
    return sock_ >= 0;
#endif
}

} // namespace qdriver::io

#include "can.hpp"

#include <cstring>

namespace qdriver::io {

Can::Can(const std::string& ifname, std::shared_ptr<qdriver::logger::Logger> logger):
    ifname_(ifname),
    logger_(logger ? logger : qdriver::logger::LoggerFactory::getDefaultLogger()) {
    logger_->info("[CAN] Initializing CAN interface: {}", ifname_);

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
}

Can::~Can() {
    if (sock_ >= 0) {
        logger_->info("[CAN] Closing CAN interface: {}", ifname_);
        close(sock_);
    }
}

bool Can::sendFrame(const std::vector<uint8_t>& data, unsigned int id) {
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
}

bool Can::receiveFrame(std::vector<uint8_t>& data, std::shared_ptr<uint32_t> id) {
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
}

bool Can::isOpen() const {
    return sock_ >= 0;
}

} // namespace qdriver::io

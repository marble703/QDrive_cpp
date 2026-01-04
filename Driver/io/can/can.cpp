#include "can.hpp"

#include <cstring>
#include <linux/can.h>
#include <linux/can/raw.h>
#include <net/if.h>
#include <stdexcept>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <unistd.h>

namespace qdriver::io {

Can::Can(const std::string& ifname, int sendCanID, int receiveCanID):
    sendCanID_(sendCanID),
    receiveCanID_(receiveCanID) {
    // 创建 Socket
    sock_ = socket(PF_CAN, SOCK_RAW, CAN_RAW);
    if (sock_ < 0) {
        throw std::runtime_error("Failed to create CAN socket");
    }

    ifreq ifr;
    std::memset(&ifr, 0, sizeof(ifr));
    std::strncpy(ifr.ifr_name, ifname.c_str(), IFNAMSIZ - 1);

    // 获取接口索引
    if (ioctl(sock_, SIOCGIFINDEX, &ifr) < 0) {
        close(sock_);
        throw std::runtime_error("Failed to get interface index for " + ifname);
    }

    sockaddr_can addr;
    std::memset(&addr, 0, sizeof(addr));
    addr.can_family  = AF_CAN;
    addr.can_ifindex = ifr.ifr_ifindex;

    // 绑定套接字
    if (bind(sock_, (sockaddr*)&addr, sizeof(addr)) < 0) {
        close(sock_);
        throw std::runtime_error("Failed to bind CAN socket to " + ifname);
    }
}

Can::~Can() {
    if (sock_ >= 0)
        close(sock_);
}

bool Can::sendFrame(const std::vector<uint8_t>& data, int id) {
    if (sock_ < 0)
        return false;

    can_frame frame;
    std::memset(&frame, 0, sizeof(frame));

    if (id < 0)
        id = this->sendCanID_;

    frame.can_id = id;

    if (data.size() > 8) {
        frame.can_dlc = 8;
    } else {
        frame.can_dlc = data.size();
    }

    std::copy(data.begin(), data.begin() + frame.can_dlc, frame.data);

    return write(sock_, &frame, sizeof(can_frame)) > 0;
}

bool Can::receiveFrame(std::vector<uint8_t>& data) {
    if (sock_ < 0)
        return false;

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
        return false;
    }

    // 文件描述符不可读
    if (!FD_ISSET(sock_, &rfds)) {
        return false;
    }

    can_frame frame;
    int nbytes = read(sock_, &frame, sizeof(can_frame));

    // 读取错误
    if (nbytes < 0) {
        return false;
    }
    // 帧不完整

    if (nbytes < (int)sizeof(can_frame)) {
        return false;
    }

    data.assign(frame.data, frame.data + frame.can_dlc);
    return true;
}

bool Can::isOpen() const {
    return sock_ >= 0;
}

} // namespace qdriver::io

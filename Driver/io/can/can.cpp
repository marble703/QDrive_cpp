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

Can::Can(const std::string& ifname) {
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

bool Can::sendFrame(uint32_t id, const std::vector<uint8_t>& data) {
    if (sock_ < 0)
        return false;

    can_frame frame;
    std::memset(&frame, 0, sizeof(frame));
    frame.can_id = id;

    if (data.size() > 8) {
        frame.can_dlc = 8;
    } else {
        frame.can_dlc = data.size();
    }

    std::copy(data.begin(), data.begin() + frame.can_dlc, frame.data);

    return write(sock_, &frame, sizeof(can_frame)) > 0;
}

bool Can::receiveFrame(uint32_t& id, std::vector<uint8_t>& data) {
    if (sock_ < 0)
        return false;

    can_frame frame;
    int nbytes = read(sock_, &frame, sizeof(can_frame));

    if (nbytes < 0) {
        // Error reading
        return false;
    }

    if (nbytes < (int)sizeof(can_frame)) {
        // Incomplete frame
        return false;
    }

    id = frame.can_id;
    data.assign(frame.data, frame.data + frame.can_dlc);
    return true;
}

bool Can::isOpen() const {
    return sock_ >= 0;
}

} // namespace qdriver::io

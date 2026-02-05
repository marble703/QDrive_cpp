#pragma once

#include <linux/can.h>
#include <net/if.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <vector>

#include "logger.hpp"

namespace qdriver::io {

class Can {
public:
    explicit Can(
        const std::string& ifname,
        std::shared_ptr<qdriver::logger::Logger> logger =
            std::make_shared<qdriver::logger::Logger>()
    );
    ~Can();

    bool sendFrame(const std::vector<uint8_t>& data, unsigned int id);
    // TODO: 接受指定 canID 数据帧
    bool receiveFrame(std::vector<uint8_t>& data, std::shared_ptr<uint32_t> id = nullptr);
    bool isOpen() const;

private:
    int sock_ { -1 };
    std::string ifname_;
    std::shared_ptr<qdriver::logger::Logger> logger_;
};

} // namespace qdriver::io
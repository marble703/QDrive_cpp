#pragma once

#include <cstdint>
#include <linux/can.h>
#include <string>
#include <vector>

namespace qdriver::io {

class Can {
public:
    explicit Can(
        const std::string& ifname,
        int sendCanId    = 0x400,
        int receiveCanId = 0x500
    );
    ~Can();

    bool sendFrame(const std::vector<uint8_t>& data = {}, int id = -1);
    // TODO: 接受指定 canID 数据帧
    bool receiveFrame(std::vector<uint8_t>& data , int id = -1);
    bool isOpen() const;

private:
    int sock_ { -1 };
    int sendCanId_ { 0x400 };
    int receiveCanId_ { 0x500 };
};

} // namespace qdriver::io
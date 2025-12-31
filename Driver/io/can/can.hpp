#pragma once

#include <string>
#include <vector>
#include <cstdint>
#include <linux/can.h>

namespace qdriver::io {

class Can {
public:
    explicit Can(const std::string& ifname);
    ~Can();

    bool sendFrame(uint32_t id, const std::vector<uint8_t>& data);
    bool receiveFrame(uint32_t& id, std::vector<uint8_t>& data);
    bool isOpen() const;

private:
    int sock_ { -1 };
};

} // namespace qdriver::io
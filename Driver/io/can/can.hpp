#pragma once

#include <linux/can.h>
#include <string>
#include <vector>
#include <memory>

namespace qdriver::io {

class Can {
public:
    explicit Can(const std::string& ifname);
    ~Can();

    bool sendFrame(const std::vector<uint8_t>& data, size_t id);
    // TODO: 接受指定 canID 数据帧
    bool receiveFrame(std::vector<uint8_t>& data, std::shared_ptr<size_t>id = nullptr);
    bool isOpen() const;

private:
    int sock_ { -1 };
};

} // namespace qdriver::io
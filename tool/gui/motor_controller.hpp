#pragma once

#include <algorithm>
#include <atomic>
#include <cctype>
#include <iostream>
#include <memory>
#include <mutex>
#include <regex>
#include <string>
#include <thread>
#include <vector>

#include "interface/interface.hpp"
#include "logger.hpp"

// Data structures
struct MotorState {
    float angle    = 0.0f;
    float speed    = 0.0f;
    float current  = 0.0f;
    uint8_t status = 0;
};

struct ConfigSnapshot {
    bool hasSpeedKp  = false;
    bool hasSpeedKi  = false;
    bool hasSpeedKd  = false;
    bool hasAngleKp  = false;
    bool hasAngleKi  = false;
    bool hasAngleKd  = false;
    bool hasLimitSpd = false;
    bool hasLimitCur = false;
    bool hasCanId    = false;
    bool hasCanBaud  = false;

    float speedKp      = 0.0f;
    float speedKi      = 0.0f;
    float speedKd      = 0.0f;
    float angleKp      = 0.0f;
    float angleKi      = 0.0f;
    float angleKd      = 0.0f;
    float limitSpeed   = 0.0f;
    float limitCurrent = 0.0f;
    int canId          = 0;
    int canBaud        = 0;
};

// Control constants
#if __cplusplus >= 202002L
constexpr float PI = std::numbers::pi_v<float>;
#else
constexpr float PI = 3.14159265358979323846f;
#endif

constexpr float MAX_CURRENT_CTRL_VALUE = 10.0f;
constexpr float MIN_CURRENT_CTRL_VALUE = -10.0f;
constexpr float MAX_SPEED_CTRL_VALUE   = 1000.0f;
constexpr float MIN_SPEED_CTRL_VALUE   = -1000.0f;
constexpr float MAX_ANGLE_CTRL_VALUE   = PI * 2;
constexpr float MIN_ANGLE_CTRL_VALUE   = 0.0f;

// Control value conversion utilities
inline float ctrlValueToCurrent(int16_t val) {
    return (static_cast<float>(val) + 32768.0f) / 65535.0f
        * (MAX_CURRENT_CTRL_VALUE - MIN_CURRENT_CTRL_VALUE)
        + MIN_CURRENT_CTRL_VALUE;
}

inline float ctrlValueToSpeed(int16_t val) {
    return (static_cast<float>(val) + 32768.0f) / 65535.0f
        * (MAX_SPEED_CTRL_VALUE - MIN_SPEED_CTRL_VALUE)
        + MIN_SPEED_CTRL_VALUE;
}

inline float ctrlValueToAngle(int16_t val) {
    return (static_cast<float>(val) + 32768.0f) / 65535.0f
        * (MAX_ANGLE_CTRL_VALUE - MIN_ANGLE_CTRL_VALUE)
        + MIN_ANGLE_CTRL_VALUE;
}

// Motor controller backend
class MotorController {
public:
    MotorController();
    ~MotorController();

    bool init(const std::string& serialDev, const std::string& canIf);
    void stop();

    // State access
    MotorState getState();

    // Control commands
    void setTargetSpeed(float s);
    void setTargetCurrent(float c);
    void setTargetAngle(float a);
    void enable();
    void disable();

    // Serial configuration
    void configSpeedPID(float kp, float ki, float kd);
    void configAnglePID(float kp, float ki, float kd);
    void configLimitSpeed(float s);
    void configLimitCurrent(float c);
    void configCanID(uint32_t id);
    void configBaudRate(uint32_t b);
    void store();
    void restore();
    void reboot();

    // Config refresh
    void requestConfigRefresh();
    ConfigSnapshot getConfigSnapshot();

    // CAN ID configuration
    void setCanIds(uint32_t send, uint32_t recv);

private:
    enum class CmdType { NONE, SPEED, CURRENT, ANGLE };
    enum class ActionType { NONE, ENABLE, DISABLE };

    // Parsing methods
    void parseSerialMessage(std::string& msg);
    void parseConfigLine(const std::string& line);
    void parseCanMessage(std::string& msg);
    void controlLoop();

    // Interfaces
    std::shared_ptr<qdriver::interface::Interface> interfaceSerial_;
    std::shared_ptr<qdriver::interface::Interface> interfaceCan_;
    std::shared_ptr<qdriver::logger::Logger> logger_;

    // State management
    std::mutex stateMutex_;
    MotorState currentState_;

    std::thread controlThread_;
    std::atomic<bool> running_{false};

    // Command management
    std::mutex cmdMutex_;
    float targetSpeed_        = 0.0f;
    float targetCurrent_      = 0.0f;
    float targetAngle_        = 0.0f;
    CmdType lastCmdType_      = CmdType::NONE;
    bool newCmd_              = false;
    ActionType pendingAction_ = ActionType::NONE;

    uint32_t sendId_ = 0x400;
    uint32_t recvId_ = 0x500;

    // Serial buffer management
    std::mutex serialMutex_;
    std::string serialBuffer_;
    bool parsedSincePrompt_  = false;
    bool parsedSinceRefresh_ = false;
    bool refreshInProgress_  = false;
    std::vector<std::string> parsedLinesThisRefresh_;
    static constexpr size_t kMaxSerialBuffer = 4096;

    // Config storage
    std::mutex configMutex_;
    ConfigSnapshot lastConfig_;
};

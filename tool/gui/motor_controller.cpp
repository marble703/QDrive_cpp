#include "motor_controller.hpp"
#include <iostream>
#include <regex>

MotorController::MotorController() {
    logger_ = std::make_shared<qdriver::logger::Logger>();
}

MotorController::~MotorController() {
    stop();
}

bool MotorController::init(const std::string& serialDev, const std::string& canIf) {
    stop();

    try {
        auto ioContextSerial =
            qdriver::io::IOContextPtrSelector(std::make_unique<boost::asio::io_context>());
        auto serialPort =
            std::make_shared<qdriver::io::Serial>(std::move(ioContextSerial), serialDev, 115200);
        interfaceSerial_ = std::make_shared<qdriver::interface::Interface>(serialPort, logger_);

        interfaceSerial_->startReaderThread([this](std::string& msg) {
            this->parseSerialMessage(msg);
        });

        auto canPort  = std::make_shared<qdriver::io::Can>(canIf, logger_);
        interfaceCan_ = std::make_shared<qdriver::interface::Interface>(canPort, logger_);

        interfaceCan_->startReaderThread([this](std::string& msg) { this->parseCanMessage(msg); });

        running_       = true;
        controlThread_ = std::thread(&MotorController::controlLoop, this);

        return true;
    } catch (const std::exception& e) {
        std::cerr << "Initialization failed: " << e.what() << std::endl;
        stop();
        return false;
    }
}

void MotorController::stop() {
    if (!running_ && !controlThread_.joinable() && !interfaceSerial_ && !interfaceCan_)
        return;

    running_ = false;

    if (controlThread_.joinable()) {
        controlThread_.join();
    }

    if (interfaceCan_) {
        try {
            interfaceCan_->ReleaseReaderThread();
        } catch (...) {}
        interfaceCan_.reset();
    }

    if (interfaceSerial_) {
        try {
            interfaceSerial_->ReleaseReaderThread();
        } catch (...) {}
        interfaceSerial_.reset();
    }

    {
        std::lock_guard<std::mutex> lock(serialMutex_);
        serialBuffer_.clear();
        parsedSincePrompt_  = false;
        parsedSinceRefresh_ = false;
        refreshInProgress_  = false;
        parsedLinesThisRefresh_.clear();
    }
}

MotorState MotorController::getState() {
    std::lock_guard<std::mutex> lock(stateMutex_);
    return currentState_;
}

void MotorController::setTargetSpeed(float s) {
    std::lock_guard<std::mutex> lock(cmdMutex_);
    targetSpeed_ = s;
    lastCmdType_ = CmdType::SPEED;
    newCmd_      = true;
}

void MotorController::setTargetCurrent(float c) {
    std::lock_guard<std::mutex> lock(cmdMutex_);
    targetCurrent_ = c;
    lastCmdType_   = CmdType::CURRENT;
    newCmd_        = true;
}

void MotorController::setTargetAngle(float a) {
    std::lock_guard<std::mutex> lock(cmdMutex_);
    targetAngle_ = a;
    lastCmdType_ = CmdType::ANGLE;
    newCmd_      = true;
}

void MotorController::enable() {
    std::lock_guard<std::mutex> lock(cmdMutex_);
    pendingAction_ = ActionType::ENABLE;
}

void MotorController::disable() {
    std::lock_guard<std::mutex> lock(cmdMutex_);
    pendingAction_ = ActionType::DISABLE;
}

void MotorController::configSpeedPID(float kp, float ki, float kd) {
    if (!interfaceSerial_)
        return;
    interfaceSerial_->configSpeed(kp, qdriver::interface::PIDtype::KP);
    interfaceSerial_->configSpeed(ki, qdriver::interface::PIDtype::KI);
    interfaceSerial_->configSpeed(kd, qdriver::interface::PIDtype::KD);
}

void MotorController::configAnglePID(float kp, float ki, float kd) {
    if (!interfaceSerial_)
        return;
    interfaceSerial_->configAngle(kp, qdriver::interface::PIDtype::KP);
    interfaceSerial_->configAngle(ki, qdriver::interface::PIDtype::KI);
    interfaceSerial_->configAngle(kd, qdriver::interface::PIDtype::KD);
}

void MotorController::configLimitSpeed(float s) {
    if (interfaceSerial_)
        interfaceSerial_->configLimitSpeed(s);
}

void MotorController::configLimitCurrent(float c) {
    if (interfaceSerial_)
        interfaceSerial_->configLimitCurrent(c);
}

void MotorController::configCanID(uint32_t id) {
    if (interfaceSerial_)
        interfaceSerial_->configCanID(id);
}

void MotorController::configBaudRate(uint32_t b) {
    if (interfaceSerial_)
        interfaceSerial_->configBaudRate(b);
}

void MotorController::store() {
    if (interfaceSerial_)
        interfaceSerial_->store();
}

void MotorController::restore() {
    if (interfaceSerial_)
        interfaceSerial_->restore();
}

void MotorController::reboot() {
    if (interfaceSerial_)
        interfaceSerial_->reboot();
}

void MotorController::requestConfigRefresh() {
    if (!interfaceSerial_)
        return;
    {
        std::lock_guard<std::mutex> lock(serialMutex_);
        refreshInProgress_  = true;
        parsedSincePrompt_  = false;
        parsedSinceRefresh_ = false;
        parsedLinesThisRefresh_.clear();
        serialBuffer_.clear();
    }
    interfaceSerial_->getConfig();
}

ConfigSnapshot MotorController::getConfigSnapshot() {
    std::lock_guard<std::mutex> lock(configMutex_);
    return lastConfig_;
}

void MotorController::setCanIds(uint32_t send, uint32_t recv) {
    sendId_ = send;
    recvId_ = recv;
}

void MotorController::parseSerialMessage(std::string& msg) {
    std::string sanitized;
    sanitized.reserve(msg.size());
    for (unsigned char c: msg) {
        if (c == '\0')
            continue;
        if (c == '\n' || c == '\r' || c == '\t' || c == ' ') {
            sanitized.push_back(static_cast<char>(c));
        } else if (std::isprint(c)) {
            sanitized.push_back(static_cast<char>(c));
        }
    }

    const std::string prompt = "QDrive:/$";
    std::lock_guard<std::mutex> lock(serialMutex_);
    serialBuffer_ += sanitized;

    if (serialBuffer_.size() > kMaxSerialBuffer) {
        serialBuffer_.erase(0, serialBuffer_.size() - kMaxSerialBuffer);
    }

    size_t lineEnd = std::string::npos;
    while ((lineEnd = serialBuffer_.find_first_of("\r\n")) != std::string::npos) {
        std::string line = serialBuffer_.substr(0, lineEnd);
        serialBuffer_.erase(0, lineEnd + 1);
        parseConfigLine(line);
    }

    auto promptPos = serialBuffer_.find(prompt);
    if (promptPos != std::string::npos) {
        if (!parsedSincePrompt_) {
            serialBuffer_.clear();
        } else {
            serialBuffer_.erase(0, promptPos + prompt.size());
        }

        if (refreshInProgress_) {
            refreshInProgress_ = false;
            parsedLinesThisRefresh_.clear();
        }

        parsedSincePrompt_  = false;
        parsedSinceRefresh_ = false;
    }
}

void MotorController::parseConfigLine(const std::string& line) {
    static const std::regex re(
        R"((pid\.speed\.kp|pid\.speed\.ki|pid\.speed\.kd|pid\.angle\.kp|pid\.angle\.ki|pid\.angle\.kd|limit\.speed|limit\.current|can\.id|can\.baud_rate|baudrate)\s*=\s*([0-9eE\+\-\.']+))"
    );
    std::smatch m;
    if (!std::regex_search(line, m, re) || m.size() < 3) {
        return;
    }

    std::string key   = m[1].str();
    std::string value = m[2].str();
    value.erase(std::remove(value.begin(), value.end(), '\''), value.end());

    bool parsed = false;
    {
        std::lock_guard<std::mutex> lock(configMutex_);
        if (key == "pid.speed.kp") {
            lastConfig_.speedKp    = std::stof(value);
            lastConfig_.hasSpeedKp = true;
            parsed                 = true;
        } else if (key == "pid.speed.ki") {
            lastConfig_.speedKi    = std::stof(value);
            lastConfig_.hasSpeedKi = true;
            parsed                 = true;
        } else if (key == "pid.speed.kd") {
            lastConfig_.speedKd    = std::stof(value);
            lastConfig_.hasSpeedKd = true;
            parsed                 = true;
        } else if (key == "pid.angle.kp") {
            lastConfig_.angleKp    = std::stof(value);
            lastConfig_.hasAngleKp = true;
            parsed                 = true;
        } else if (key == "pid.angle.ki") {
            lastConfig_.angleKi    = std::stof(value);
            lastConfig_.hasAngleKi = true;
            parsed                 = true;
        } else if (key == "pid.angle.kd") {
            lastConfig_.angleKd    = std::stof(value);
            lastConfig_.hasAngleKd = true;
            parsed                 = true;
        } else if (key == "limit.speed") {
            lastConfig_.limitSpeed  = std::stof(value);
            lastConfig_.hasLimitSpd = true;
            parsed                  = true;
        } else if (key == "limit.current") {
            lastConfig_.limitCurrent = std::stof(value);
            lastConfig_.hasLimitCur  = true;
            parsed                   = true;
        } else if (key == "can.id") {
            lastConfig_.canId    = std::stoi(value, nullptr, 0);
            lastConfig_.hasCanId = true;
            parsed               = true;
        } else if (key == "can.baud_rate" || key == "baudrate") {
            lastConfig_.canBaud    = std::stoi(value, nullptr, 0);
            lastConfig_.hasCanBaud = true;
            parsed                 = true;
        }
    }

    if (parsed) {
        parsedSincePrompt_  = true;
        parsedSinceRefresh_ = true;
        if (refreshInProgress_) {
            parsedLinesThisRefresh_.push_back(key + " = " + value);
        }
    }
}

void MotorController::parseCanMessage(std::string& msg) {
    try {
        size_t colonPos = msg.find(':');
        if (colonPos == std::string::npos)
            return;

        std::string idStr   = msg.substr(0, colonPos);
        std::string dataStr = msg.substr(colonPos + 1);

        uint32_t id = std::stoul(idStr);
        if (id != recvId_)
            return;

        std::vector<uint8_t> data;
        for (size_t i = 0; i < dataStr.length(); i += 2) {
            std::string byteString = dataStr.substr(i, 2);
            data.push_back((uint8_t)strtol(byteString.c_str(), nullptr, 16));
        }

        if (data.size() < 8)
            return;

        int16_t rawCurrent = (int16_t)(data[2] | (data[3] << 8));
        int16_t rawSpeed   = (int16_t)(data[4] | (data[5] << 8));
        int16_t rawAngle   = (int16_t)(data[6] | (data[7] << 8));

        MotorState newState;
        newState.status  = data[0];
        newState.current = ctrlValueToCurrent(rawCurrent);
        newState.speed   = ctrlValueToSpeed(rawSpeed);
        newState.angle   = ctrlValueToAngle(rawAngle);

        {
            std::lock_guard<std::mutex> lock(stateMutex_);
            currentState_ = newState;
        }
    } catch (...) {}
}

void MotorController::controlLoop() {
    while (running_) {
        auto start = std::chrono::steady_clock::now();

        {
            std::lock_guard<std::mutex> lock(cmdMutex_);
            uint32_t id = sendId_;

            if (pendingAction_ == ActionType::ENABLE) {
                interfaceCan_->enable(id);
                pendingAction_ = ActionType::NONE;
            } else if (pendingAction_ == ActionType::DISABLE) {
                interfaceCan_->disable(id);
                pendingAction_ = ActionType::NONE;
            }

            if (lastCmdType_ == CmdType::SPEED) {
                interfaceCan_->ctrlSpeed(targetSpeed_, id);
            } else if (lastCmdType_ == CmdType::CURRENT) {
                interfaceCan_->ctrlCurrent(targetCurrent_, id);
            } else if (lastCmdType_ == CmdType::ANGLE) {
                interfaceCan_->ctrlAngle(targetAngle_, id);
            } else {
                interfaceCan_->status(id);
            }
        }

        auto end     = std::chrono::steady_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();
        if (elapsed < 1000) {
            std::this_thread::sleep_for(std::chrono::microseconds(1000 - elapsed));
        }
    }
}

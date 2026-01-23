#include <fstream>
#include <iostream>
#include <regex>

#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include <GLFW/glfw3.h>

#include "interface/interface.hpp"

// Forward declaration of helper
static void glfw_error_callback(int error, const char* description) {
    fprintf(stderr, "Glfw Error %d: %s\n", error, description);
}

// Data structures for visualization
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

#if __cplusplus >= 202002L
constexpr float PI = std::numbers::pi_v<float>; // c++20
#else
constexpr float PI = 3.14159265358979323846f;
#endif

const float MAX_CURRENT_CTRL_VALUE = 10;
const float MIN_CURRENT_CTRL_VALUE = -10;
const float MAX_SPEED_CTRL_VALUE   = 1000;
const float MIN_SPEED_CTRL_VALUE   = -1000;
const float MAX_ANGLE_CTRL_VALUE   = PI * 2;
const float MIN_ANGLE_CTRL_VALUE   = 0;

float ctrlValueToCurrent(int16_t val) {
    return (static_cast<float>(val) + 32768.0f) / 65535.0f
        * (MAX_CURRENT_CTRL_VALUE - MIN_CURRENT_CTRL_VALUE)
        + MIN_CURRENT_CTRL_VALUE;
}

float ctrlValueToSpeed(int16_t val) {
    return (static_cast<float>(val) + 32768.0f) / 65535.0f
        * (MAX_SPEED_CTRL_VALUE - MIN_SPEED_CTRL_VALUE)
        + MIN_SPEED_CTRL_VALUE;
}

float ctrlValueToAngle(int16_t val) {
    return (static_cast<float>(val) + 32768.0f) / 65535.0f
        * (MAX_ANGLE_CTRL_VALUE - MIN_ANGLE_CTRL_VALUE)
        + MIN_ANGLE_CTRL_VALUE;
}

class MotorController {
public:
    MotorController() {
        // Initialize Logger
        logger_ = std::make_shared<qdriver::logger::Logger>();
    }

    ~MotorController() {
        stop();
    }

    bool init(const std::string& serialDev, const std::string& canIf) {
        // Cleanup any existing resources first
        stop();

        try {
            // Setup Serial
            auto ioContextSerial =
                qdriver::io::IOContextPtrSelector(std::make_unique<boost::asio::io_context>());
            auto serialPort = std::make_shared<qdriver::io::Serial>(
                std::move(ioContextSerial),
                serialDev,
                115200
            );
            interfaceSerial_ = std::make_shared<qdriver::interface::Interface>(serialPort, logger_);

            // Start Serial reader
            interfaceSerial_->startReaderThread([this](std::string& msg) {
                this->parseSerialMessage(msg);
            });

            // Setup CAN
            auto canPort  = std::make_shared<qdriver::io::Can>(canIf, logger_);
            interfaceCan_ = std::make_shared<qdriver::interface::Interface>(canPort, logger_);

            // Start CAN reader
            interfaceCan_->startReaderThread([this](std::string& msg) {
                this->parseCanMessage(msg);
            });

            running_       = true;
            controlThread_ = std::thread(&MotorController::controlLoop, this);

            return true;
        } catch (const std::exception& e) {
            std::cerr << "Initialization failed: " << e.what() << std::endl;
            // Cleanup partial initialization
            stop();
            return false;
        }
    }

    void stop() {
        if (!running_ && !controlThread_.joinable() && !interfaceSerial_ && !interfaceCan_)
            return; // Already stopped

        running_ = false;

        // Stop control thread
        if (controlThread_.joinable()) {
            controlThread_.join();
        }

        // Stop reader threads and release interfaces
        if (interfaceCan_) {
            try {
                interfaceCan_->ReleaseReaderThread();
            } catch (...) {
                // Ignore cleanup errors
            }
            interfaceCan_.reset();
        }

        if (interfaceSerial_) {
            try {
                interfaceSerial_->ReleaseReaderThread();
            } catch (...) {
                // Ignore cleanup errors
            }
            interfaceSerial_.reset();
        }

        // Clear serial buffer state to prevent segfault on reconnect
        {
            std::lock_guard<std::mutex> lock(serialMutex_);
            serialBuffer_.clear();
            parsedSincePrompt_  = false;
            parsedSinceRefresh_ = false;
            refreshInProgress_  = false;
            parsedLinesThisRefresh_.clear();
        }
    }

    // Thread-safe state access
    MotorState getState() {
        std::lock_guard<std::mutex> lock(stateMutex_);
        return currentState_;
    }

    void setTargetSpeed(float s) {
        std::lock_guard<std::mutex> lock(cmdMutex_);
        targetSpeed_ = s;
        lastCmdType_ = CmdType::SPEED;
        newCmd_      = true;
    }
    void setTargetCurrent(float c) {
        std::lock_guard<std::mutex> lock(cmdMutex_);
        targetCurrent_ = c;
        lastCmdType_   = CmdType::CURRENT;
        newCmd_        = true;
    }
    void setTargetAngle(float a) {
        std::lock_guard<std::mutex> lock(cmdMutex_);
        targetAngle_ = a;
        lastCmdType_ = CmdType::ANGLE;
        newCmd_      = true;
    }

    // Command sending wrappers
    void enable() {
        std::lock_guard<std::mutex> lock(cmdMutex_);
        pendingAction_ = ActionType::ENABLE;
    }
    void disable() {
        std::lock_guard<std::mutex> lock(cmdMutex_);
        pendingAction_ = ActionType::DISABLE;
    }

    // Serial configs
    void configSpeedPID(float kp, float ki, float kd) {
        if (!interfaceSerial_)
            return;
        interfaceSerial_->configSpeed(kp, qdriver::interface::PIDtype::KP);
        interfaceSerial_->configSpeed(ki, qdriver::interface::PIDtype::KI);
        interfaceSerial_->configSpeed(kd, qdriver::interface::PIDtype::KD);
    }

    void configAnglePID(float kp, float ki, float kd) {
        if (!interfaceSerial_)
            return;
        interfaceSerial_->configAngle(kp, qdriver::interface::PIDtype::KP);
        interfaceSerial_->configAngle(ki, qdriver::interface::PIDtype::KI);
        interfaceSerial_->configAngle(kd, qdriver::interface::PIDtype::KD);
    }

    void configLimitSpeed(float s) {
        if (interfaceSerial_)
            interfaceSerial_->configLimitSpeed(s);
    }
    void configLimitCurrent(float c) {
        if (interfaceSerial_)
            interfaceSerial_->configLimitCurrent(c);
    }
    void configCanID(uint32_t id) {
        if (interfaceSerial_)
            interfaceSerial_->configCanID(id);
    }
    void configBaudRate(uint32_t b) {
        if (interfaceSerial_)
            interfaceSerial_->configBaudRate(b);
    }

    void store() {
        if (interfaceSerial_)
            interfaceSerial_->store();
    }
    void restore() {
        if (interfaceSerial_)
            interfaceSerial_->restore();
    }
    void reboot() {
        if (interfaceSerial_)
            interfaceSerial_->reboot();
    }

    void requestConfigRefresh() {
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
        // Debug output suppressed
        // std::cout << "[ConfigRefresh] Requesting config list..." << std::endl;
        interfaceSerial_->getConfig();
    }

    ConfigSnapshot getConfigSnapshot() {
        std::lock_guard<std::mutex> lock(configMutex_);
        return lastConfig_;
    }

private:
    enum class CmdType { NONE, SPEED, CURRENT, ANGLE };
    enum class ActionType { NONE, ENABLE, DISABLE };

    std::shared_ptr<qdriver::interface::Interface> interfaceSerial_;
    std::shared_ptr<qdriver::interface::Interface> interfaceCan_;
    std::shared_ptr<qdriver::logger::Logger> logger_;

    std::mutex stateMutex_;
    MotorState currentState_;

    std::thread controlThread_;
    std::atomic<bool> running_ { false };

    std::mutex cmdMutex_;
    float targetSpeed_        = 0.0f;
    float targetCurrent_      = 0.0f;
    float targetAngle_        = 0.0f;
    CmdType lastCmdType_      = CmdType::NONE;
    bool newCmd_              = false;
    ActionType pendingAction_ = ActionType::NONE;

    uint32_t sendId_ = 0x400;
    uint32_t recvId_ = 0x500;

    std::mutex serialMutex_;
    std::string serialBuffer_;
    bool parsedSincePrompt_  = false;
    bool parsedSinceRefresh_ = false;
    bool refreshInProgress_  = false;
    std::vector<std::string> parsedLinesThisRefresh_;
    static constexpr size_t kMaxSerialBuffer = 4096;

    std::mutex configMutex_;
    ConfigSnapshot lastConfig_;

public:
    void setCanIds(uint32_t send, uint32_t recv) {
        sendId_ = send;
        recvId_ = recv;
    }

private:
    void parseSerialMessage(std::string& msg) {
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

        // Process complete lines
        size_t lineEnd = std::string::npos;
        while ((lineEnd = serialBuffer_.find_first_of("\r\n")) != std::string::npos) {
            std::string line = serialBuffer_.substr(0, lineEnd);
            serialBuffer_.erase(0, lineEnd + 1);
            parseConfigLine(line);
        }

        // Handle prompt detection (may arrive without newline)
        auto promptPos = serialBuffer_.find(prompt);
        if (promptPos != std::string::npos) {
            if (!parsedSincePrompt_) {
                serialBuffer_.clear();

            } else {
                serialBuffer_.erase(0, promptPos + prompt.size());
            }

            if (refreshInProgress_) {
                for (const auto& ln: parsedLinesThisRefresh_) {}
                refreshInProgress_ = false;
                parsedLinesThisRefresh_.clear();
            }

            parsedSincePrompt_  = false;
            parsedSinceRefresh_ = false;
        }
    }

    void parseConfigLine(const std::string& line) {
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

    void parseCanMessage(std::string& msg) {
        // Msg format "ID:HEXDATA" (e.g. "1280:AABBCC...")
        try {
            size_t colonPos = msg.find(':');
            if (colonPos == std::string::npos)
                return;

            std::string idStr   = msg.substr(0, colonPos);
            std::string dataStr = msg.substr(colonPos + 1);

            uint32_t id = std::stoul(idStr);
            if (id != recvId_)
                return; // Filter by expected ID

            // Hex string to bytes
            std::vector<uint8_t> data;
            for (size_t i = 0; i < dataStr.length(); i += 2) {
                std::string byteString = dataStr.substr(i, 2);
                data.push_back((uint8_t)strtol(byteString.c_str(), nullptr, 16));
            }

            if (data.size() < 8)
                return;

            // Parse bytes: 0:Status, 1:Res, 2-3:Cur, 4-5:Spd, 6-7:Ang
            // Little Endian
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

        } catch (...) {
            // Ignore parsing errors
        }
    }

    void controlLoop() {
        while (running_) {
            auto start = std::chrono::steady_clock::now();

            {
                std::lock_guard<std::mutex> lock(cmdMutex_);
                uint32_t id = sendId_;

                // One-off actions
                if (pendingAction_ == ActionType::ENABLE) {
                    interfaceCan_->enable(id);
                    pendingAction_ = ActionType::NONE;
                } else if (pendingAction_ == ActionType::DISABLE) {
                    interfaceCan_->disable(id);
                    pendingAction_ = ActionType::NONE;
                }

                // Continuous control
                if (lastCmdType_ == CmdType::SPEED) {
                    interfaceCan_->ctrlSpeed(targetSpeed_, id);
                } else if (lastCmdType_ == CmdType::CURRENT) {
                    interfaceCan_->ctrlCurrent(targetCurrent_, id);
                } else if (lastCmdType_ == CmdType::ANGLE) {
                    interfaceCan_->ctrlAngle(targetAngle_, id);
                } else {
                    // If no command, query status
                    interfaceCan_->status(id);
                }
            }

            auto end = std::chrono::steady_clock::now();
            auto elapsed =
                std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();
            if (elapsed < 1000) {
                std::this_thread::sleep_for(std::chrono::microseconds(1000 - elapsed));
            }
        }
    }
};

static void PlotLinesWithGrid(
    const char* label,
    const float* values,
    int values_count,
    float scale_min,
    float scale_max,
    ImVec2 size = ImVec2(0, 80),
    int grid_x  = 10,
    int grid_y  = 5
) {
    if (values_count <= 0)
        return;
    if (size.x == 0.0f)
        size.x = ImGui::GetContentRegionAvail().x - 100; // 右侧留出

    ImVec2 pos = ImGui::GetCursorScreenPos();
    ImVec2 br(pos.x + size.x, pos.y + size.y);
    ImDrawList* dl = ImGui::GetWindowDrawList();

    // 网格颜色与边框
    ImU32 gridCol = ImGui::GetColorU32(ImVec4(1, 1, 1, 0.06f));

    for (int i = 1; i < grid_x; ++i) {
        float x = pos.x + (float)i / (float)grid_x * size.x;
        dl->AddLine(ImVec2(x, pos.y), ImVec2(x, br.y), gridCol);
    }
    for (int j = 1; j < grid_y; ++j) {
        float y = pos.y + (float)j / (float)grid_y * size.y;
        dl->AddLine(ImVec2(pos.x, y), ImVec2(br.x, y), gridCol);
    }

    ImGui::PlotLines(label, values, values_count, 0, NULL, scale_min, scale_max, size);
}

// Simple config file helper
struct AppConfig {
    std::string serialPort   = "/dev/ttyACM0";
    std::string canInterface = "can0";
    int sendIdHex            = 0x400;
    int recvIdHex            = 0x500;
};

void saveConfig(const AppConfig& cfg, const std::string& filename = "qdrive_gui.conf") {
    std::ofstream f(filename);
    if (f.is_open()) {
        f << "serial_port=" << cfg.serialPort << "\n";
        f << "can_interface=" << cfg.canInterface << "\n";
        f << "send_id=0x" << std::hex << cfg.sendIdHex << "\n";
        f << "recv_id=0x" << std::hex << cfg.recvIdHex << "\n";
    }
}

AppConfig loadConfig(const std::string& filename = "qdrive_gui.conf") {
    AppConfig cfg;
    std::ifstream f(filename);
    if (f.is_open()) {
        std::string line;
        while (std::getline(f, line)) {
            auto pos = line.find('=');
            if (pos == std::string::npos)
                continue;
            std::string key = line.substr(0, pos);
            std::string val = line.substr(pos + 1);
            if (key == "serial_port")
                cfg.serialPort = val;
            else if (key == "can_interface")
                cfg.canInterface = val;
            else if (key == "send_id")
                cfg.sendIdHex = std::stoi(val, nullptr, 16);
            else if (key == "recv_id")
                cfg.recvIdHex = std::stoi(val, nullptr, 16);
        }
    }
    return cfg;
}

int main(int, char**) {
    glfwSetErrorCallback(glfw_error_callback);
    if (!glfwInit())
        return 1;

    const char* glsl_version = "#version 130";
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);

    GLFWwindow* window = glfwCreateWindow(1280, 720, "QDrive Motor Tuner", NULL, NULL);
    if (window == NULL)
        return 1;
    glfwMakeContextCurrent(window);
    glfwSwapInterval(1); // Enable vsync

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    (void)io;
    ImGui::StyleColorsDark();

    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init(glsl_version);

    // App state
    MotorController motor;
    bool connected = false;

    // Load config from file
    AppConfig appCfg = loadConfig();
    char serialDev[64];
    char canIf[64];
    std::strncpy(serialDev, appCfg.serialPort.c_str(), 63);
    serialDev[63] = '\0';
    std::strncpy(canIf, appCfg.canInterface.c_str(), 63);
    canIf[63] = '\0';

    // Control vars
    float targetSpeed   = 0.0f;
    float targetAngle   = 0.0f;
    float targetCurrent = 0.0f;
    int controlMode     = 0; // 0: None/Status, 1: Speed, 2: Angle, 3: Current

    // ID config
    int sendIdInput = appCfg.sendIdHex;
    int recvIdInput = appCfg.recvIdHex;

    // PID vars
    float speedKP = 0, speedKI = 0, speedKD = 0;
    float angleKP = 0, angleKI = 0, angleKD = 0;

    float limitSpeed   = 1000.0f;
    float limitCurrent = 10.0f;
    int configCanId    = 0;
    int configBaud     = 115200;

    // Plotting buffers
    std::vector<float> speedHistory(200, 0);
    std::vector<float> angleHistory(200, 0);
    std::vector<float> currentHistory(200, 0);
    size_t plotIdx = 0;

    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        ImGui::Begin("Motor Control");

        // Connection
        if (!connected) {
            ImGui::InputText("Serial Port", serialDev, 64);
            ImGui::InputText("CAN Interface", canIf, 64);
            ImGui::InputInt(
                "Send ID (Hex)",
                &sendIdInput,
                1,
                100,
                ImGuiInputTextFlags_CharsHexadecimal
            );
            ImGui::InputInt(
                "Recv ID (Hex)",
                &recvIdInput,
                1,
                100,
                ImGuiInputTextFlags_CharsHexadecimal
            );

            if (ImGui::Button("Connect")) {
                motor.setCanIds(
                    static_cast<uint32_t>(sendIdInput),
                    static_cast<uint32_t>(recvIdInput)
                );
                if (motor.init(serialDev, canIf)) {
                    connected = true;
                    // Save config
                    appCfg.serialPort   = serialDev;
                    appCfg.canInterface = canIf;
                    appCfg.sendIdHex    = sendIdInput;
                    appCfg.recvIdHex    = recvIdInput;
                    saveConfig(appCfg);
                }
            }
        } else {
            ImGui::Text("Connected to %s and %s", serialDev, canIf);
            if (ImGui::Button("Disconnect")) {
                motor.stop();
                connected = false;
                // Note: simplified logic, usually need to reconstruct controller
            }
        }

        ImGui::Separator();

        // Status view
        MotorState state = motor.getState();
        ImGui::Text("Status: %d", state.status);
        ImGui::Text("Speed: %.2f rpm", state.speed);
        ImGui::Text("Angle: %.2f rad", state.angle);
        ImGui::Text("Current: %.2f A", state.current);

        // Update history
        if (connected) {
            speedHistory.erase(speedHistory.begin());
            speedHistory.push_back(state.speed);
            angleHistory.erase(angleHistory.begin());
            angleHistory.push_back(state.angle);
            currentHistory.erase(currentHistory.begin());
            currentHistory.push_back(state.current);
        }

        PlotLinesWithGrid(
            "Speed (rpm)",
            speedHistory.data(),
            speedHistory.size(),
            -1000,
            1000,
            ImVec2(0, 160)
        );
        PlotLinesWithGrid(
            "Angle (rad)",
            angleHistory.data(),
            angleHistory.size(),
            0,
            PI * 2,
            ImVec2(0, 160)
        );
        PlotLinesWithGrid(
            "Current (A)",
            currentHistory.data(),
            currentHistory.size(),
            -10,
            10,
            ImVec2(0, 160)
        );

        ImGui::Separator();

        // Control
        if (connected) {
            if (ImGui::Button("Enable"))
                motor.enable();
            ImGui::SameLine();
            if (ImGui::Button("Disable"))
                motor.disable();
            ImGui::SameLine();
            if (ImGui::Button("Reboot"))
                motor.reboot();

            ImGui::RadioButton("Monitor", &controlMode, 0);
            ImGui::SameLine();
            ImGui::RadioButton("Speed Control", &controlMode, 1);
            ImGui::SameLine();
            ImGui::RadioButton("Angle Control", &controlMode, 2);
            ImGui::SameLine();
            ImGui::RadioButton("Current Control", &controlMode, 3);

            if (controlMode == 1) {
                bool sliderChanged = ImGui::SliderFloat(
                    "Target Speed",
                    &targetSpeed,
                    MIN_SPEED_CTRL_VALUE,
                    MAX_SPEED_CTRL_VALUE
                );
                ImGui::SameLine();
                bool inputChanged =
                    ImGui::InputFloat("##SpeedInput", &targetSpeed, 1.0f, 10.0f, "%.2f");
                if (inputChanged) {
                    targetSpeed =
                        std::clamp(targetSpeed, MIN_SPEED_CTRL_VALUE, MAX_SPEED_CTRL_VALUE);
                }
                if (sliderChanged || inputChanged) {
                    motor.setTargetSpeed(targetSpeed);
                }
            } else if (controlMode == 2) {
                bool sliderChanged = ImGui::SliderFloat(
                    "Target Angle",
                    &targetAngle,
                    MIN_ANGLE_CTRL_VALUE,
                    MAX_ANGLE_CTRL_VALUE
                );
                ImGui::SameLine();
                bool inputChanged =
                    ImGui::InputFloat("##AngleInput", &targetAngle, 0.1f, 1.0f, "%.6f");
                if (inputChanged) {
                    targetAngle =
                        std::clamp(targetAngle, MIN_ANGLE_CTRL_VALUE, MAX_ANGLE_CTRL_VALUE);
                }
                if (sliderChanged || inputChanged) {
                    motor.setTargetAngle(targetAngle);
                }
            } else if (controlMode == 3) {
                bool sliderChanged = ImGui::SliderFloat(
                    "Target Current",
                    &targetCurrent,
                    MIN_CURRENT_CTRL_VALUE,
                    MAX_CURRENT_CTRL_VALUE
                );
                ImGui::SameLine();
                bool inputChanged =
                    ImGui::InputFloat("##CurrentInput", &targetCurrent, 0.1f, 1.0f, "%.2f");
                if (inputChanged) {
                    targetCurrent =
                        std::clamp(targetCurrent, MIN_CURRENT_CTRL_VALUE, MAX_CURRENT_CTRL_VALUE);
                }
                if (sliderChanged || inputChanged) {
                    motor.setTargetCurrent(targetCurrent);
                }
            } else {
                // Monitor mode send no control loop commands
            }
        }

        ImGui::Separator();

        // Configuration (Serial)
        if (ImGui::CollapsingHeader("PID Configuration")) {
            if (ImGui::Button("Refresh Config")) {
                motor.requestConfigRefresh();
            }

            ConfigSnapshot cfg = motor.getConfigSnapshot();
            bool hasAnyConfig = cfg.hasSpeedKp || cfg.hasSpeedKi || cfg.hasSpeedKd || cfg.hasAngleKp
                || cfg.hasAngleKi || cfg.hasAngleKd || cfg.hasLimitSpd || cfg.hasLimitCur
                || cfg.hasCanId || cfg.hasCanBaud;
            if (hasAnyConfig) {
                ImGui::Text("Parsed Config:");
                if (cfg.hasSpeedKp)
                    ImGui::Text("pid.speed.kp = %.6f", cfg.speedKp);
                if (cfg.hasSpeedKi)
                    ImGui::Text("pid.speed.ki = %.6f", cfg.speedKi);
                if (cfg.hasSpeedKd)
                    ImGui::Text("pid.speed.kd = %.6f", cfg.speedKd);
                if (cfg.hasAngleKp)
                    ImGui::Text("pid.angle.kp = %.6f", cfg.angleKp);
                if (cfg.hasAngleKi)
                    ImGui::Text("pid.angle.ki = %.6f", cfg.angleKi);
                if (cfg.hasAngleKd)
                    ImGui::Text("pid.angle.kd = %.6f", cfg.angleKd);
                if (cfg.hasLimitSpd)
                    ImGui::Text("limit.speed = %.2f", cfg.limitSpeed);
                if (cfg.hasLimitCur)
                    ImGui::Text("limit.current = %.2f", cfg.limitCurrent);
                if (cfg.hasCanId)
                    ImGui::Text("can.id = %d", cfg.canId);
                if (cfg.hasCanBaud)
                    ImGui::Text("can.baud_rate = %d", cfg.canBaud);
            } else {
                ImGui::TextDisabled("No config parsed yet.");
            }

            ImGui::SameLine();
            if (ImGui::Button("Apply Config to Inputs")) {
                ConfigSnapshot snap = motor.getConfigSnapshot();
                bool anyCfg         = snap.hasSpeedKp || snap.hasSpeedKi || snap.hasSpeedKd
                    || snap.hasAngleKp || snap.hasAngleKi || snap.hasAngleKd || snap.hasLimitSpd
                    || snap.hasLimitCur || snap.hasCanId || snap.hasCanBaud;
                if (anyCfg) {
                    if (snap.hasSpeedKp)
                        speedKP = snap.speedKp;
                    if (snap.hasSpeedKi)
                        speedKI = snap.speedKi;
                    if (snap.hasSpeedKd)
                        speedKD = snap.speedKd;
                    if (snap.hasAngleKp)
                        angleKP = snap.angleKp;
                    if (snap.hasAngleKi)
                        angleKI = snap.angleKi;
                    if (snap.hasAngleKd)
                        angleKD = snap.angleKd;
                    if (snap.hasLimitSpd)
                        limitSpeed = snap.limitSpeed;
                    if (snap.hasLimitCur)
                        limitCurrent = snap.limitCurrent;
                    if (snap.hasCanId)
                        configCanId = snap.canId;
                    if (snap.hasCanBaud)
                        configBaud = snap.canBaud;
                    std::cout << "[ConfigApply] Applied config to UI inputs." << std::endl;
                } else {
                    std::cout << "[ConfigApply] No parsed config to apply." << std::endl;
                }
            }
            ImGui::SameLine();
            if (ImGui::Button("Send Config to Motor")) {
                motor.configSpeedPID(speedKP, speedKI, speedKD);
                motor.configAnglePID(angleKP, angleKI, angleKD);
                motor.configLimitSpeed(limitSpeed);
                motor.configLimitCurrent(limitCurrent);
                motor.configCanID((uint32_t)configCanId);
                motor.configBaudRate((uint32_t)configBaud);
                std::cout << "[ConfigSend] Sent config to motor." << std::endl;
            }

            ImGui::Text("Speed PID");
            ImGui::InputFloat("S-KP", &speedKP);
            ImGui::InputFloat("S-KI", &speedKI);
            ImGui::InputFloat("S-KD", &speedKD);
            if (ImGui::Button("Apply Speed PID")) {
                motor.configSpeedPID(speedKP, speedKI, speedKD);
            }

            ImGui::Separator();
            ImGui::Text("Angle PID");
            ImGui::InputFloat("A-KP", &angleKP);
            ImGui::InputFloat("A-KI", &angleKI);
            ImGui::InputFloat("A-KD", &angleKD);
            if (ImGui::Button("Apply Angle PID")) {
                motor.configAnglePID(angleKP, angleKI, angleKD);
            }

            ImGui::Separator();
            ImGui::Text("Limits & Sys Config");
            ImGui::InputFloat("Limit Speed", &limitSpeed);
            if (ImGui::Button("Set Speed Limit"))
                motor.configLimitSpeed(limitSpeed);

            ImGui::InputFloat("Limit Current", &limitCurrent);
            if (ImGui::Button("Set Current Limit"))
                motor.configLimitCurrent(limitCurrent);

            ImGui::InputInt("New CAN ID", &configCanId);
            if (ImGui::Button("Set CAN ID (0-8)"))
                motor.configCanID((uint32_t)configCanId);

            ImGui::InputInt("Baud Rate", &configBaud);
            if (ImGui::Button("Set Baud Rate"))
                motor.configBaudRate((uint32_t)configBaud);
        }

        if (ImGui::Button("Store Config"))
            motor.store();
        ImGui::SameLine();
        if (ImGui::Button("Restore Config"))
            motor.restore();

        ImGui::End();

        ImGui::Render();
        int display_w, display_h;
        glfwGetFramebufferSize(window, &display_w, &display_h);
        glViewport(0, 0, display_w, display_h);
        glClearColor(0.45f, 0.55f, 0.60f, 1.00f);
        glClear(GL_COLOR_BUFFER_BIT);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        glfwSwapBuffers(window);
    }

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    glfwDestroyWindow(window);
    glfwTerminate();

    return 0;
}

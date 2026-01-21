#include <iostream>
#include <vector>
#include <string>
#include <thread>
#include <mutex>
#include <atomic>
#include <chrono>
#include <cmath>


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
    float angle = 0.0f;
    float speed = 0.0f;
    float current = 0.0f;
    uint8_t status = 0;
};

// Utilities for conversion (copied from interface.hpp logic)
constexpr float PI = 3.14159265358979323846f;
const float MAX_CURRENT_CTRL_VALUE   = 10;
const float MIN_CURRENT_CTRL_VALUE   = -10;
const float MAX_SPEED_CTRL_VALUE     = 1000;
const float MIN_SPEED_CTRL_VALUE     = -1000;
const float MAX_ANGLE_CTRL_VALUE     = PI * 2;
const float MIN_ANGLE_CTRL_VALUE     = 0;

float ctrlValueToCurrent(int16_t val) {
    return (static_cast<float>(val) + 32768.0f) / 65535.0f * (MAX_CURRENT_CTRL_VALUE - MIN_CURRENT_CTRL_VALUE) + MIN_CURRENT_CTRL_VALUE;
}

float ctrlValueToSpeed(int16_t val) {
    return (static_cast<float>(val) + 32768.0f) / 65535.0f * (MAX_SPEED_CTRL_VALUE - MIN_SPEED_CTRL_VALUE) + MIN_SPEED_CTRL_VALUE;
}

float ctrlValueToAngle(int16_t val) {
    return (static_cast<float>(val) + 32768.0f) / 65535.0f * (MAX_ANGLE_CTRL_VALUE - MIN_ANGLE_CTRL_VALUE) + MIN_ANGLE_CTRL_VALUE;
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
        try {
            // Setup Serial
            auto ioContextSerial = qdriver::io::IOContextPtrSelector(std::make_unique<boost::asio::io_context>());
            auto serialPort = std::make_shared<qdriver::io::Serial>(
                std::move(ioContextSerial), serialDev, 115200
            );
            interfaceSerial_ = std::make_shared<qdriver::interface::Interface>(serialPort, logger_);
            
            // Setup CAN
            auto canPort = std::make_shared<qdriver::io::Can>(canIf, logger_);
            interfaceCan_ = std::make_shared<qdriver::interface::Interface>(canPort, logger_);

            // Start CAN reader
            interfaceCan_->startReaderThread([this](std::string& msg) {
                this->parseCanMessage(msg);
            });

            running_ = true;
            controlThread_ = std::thread(&MotorController::controlLoop, this);
            
            return true;
        } catch (const std::exception& e) {
            std::cerr << "Initialization failed: " << e.what() << std::endl;
            return false;
        }
    }

    void stop() {
        running_ = false;
        if (controlThread_.joinable()) controlThread_.join();
        if (interfaceCan_) interfaceCan_->ReleaseReaderThread();
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
        newCmd_ = true;
    }
    void setTargetCurrent(float c) { 
        std::lock_guard<std::mutex> lock(cmdMutex_);
        targetCurrent_ = c; 
        lastCmdType_ = CmdType::CURRENT;
        newCmd_ = true;
    }
    void setTargetAngle(float a) { 
        std::lock_guard<std::mutex> lock(cmdMutex_);
        targetAngle_ = a; 
        lastCmdType_ = CmdType::ANGLE;
        newCmd_ = true;
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
        if (!interfaceSerial_) return;
        interfaceSerial_->configSpeed(kp, qdriver::interface::PIDtype::KP);
        interfaceSerial_->configSpeed(ki, qdriver::interface::PIDtype::KI);
        interfaceSerial_->configSpeed(kd, qdriver::interface::PIDtype::KD);
    }
    
    void configAnglePID(float kp, float ki, float kd) {
        if (!interfaceSerial_) return;
        interfaceSerial_->configAngle(kp, qdriver::interface::PIDtype::KP);
        interfaceSerial_->configAngle(ki, qdriver::interface::PIDtype::KI);
        interfaceSerial_->configAngle(kd, qdriver::interface::PIDtype::KD);
    }
    
    void configLimitSpeed(float s) { if(interfaceSerial_) interfaceSerial_->configLimitSpeed(s); }
    void configLimitCurrent(float c) { if(interfaceSerial_) interfaceSerial_->configLimitCurrent(c); }
    void configCanID(uint32_t id) { if(interfaceSerial_) interfaceSerial_->configCanID(id); }
    void configBaudRate(uint32_t b) { if(interfaceSerial_) interfaceSerial_->configBaudRate(b); }

    void store() { if(interfaceSerial_) interfaceSerial_->store(); }
    void restore() { if(interfaceSerial_) interfaceSerial_->restore(); }
    void silent() { if(interfaceSerial_) interfaceSerial_->silent(); }
    void reboot() { if(interfaceSerial_) interfaceSerial_->reboot(); }

private:
    enum class CmdType { NONE, SPEED, CURRENT, ANGLE };
    enum class ActionType { NONE, ENABLE, DISABLE };

    std::shared_ptr<qdriver::interface::Interface> interfaceSerial_;
    std::shared_ptr<qdriver::interface::Interface> interfaceCan_;
    std::shared_ptr<qdriver::logger::Logger> logger_;

    std::mutex stateMutex_;
    MotorState currentState_;

    std::thread controlThread_;
    std::atomic<bool> running_{false};
    
    std::mutex cmdMutex_;
    float targetSpeed_ = 0.0f;
    float targetCurrent_ = 0.0f;
    float targetAngle_ = 0.0f;
    CmdType lastCmdType_ = CmdType::NONE;
    bool newCmd_ = false;
    ActionType pendingAction_ = ActionType::NONE;
    
    uint32_t sendId_ = 0x400;
    uint32_t recvId_ = 0x500;

public:
    void setCanIds(uint32_t send, uint32_t recv) {
        sendId_ = send;
        recvId_ = recv;
    }

private:
    void parseCanMessage(std::string& msg) {
        // Msg format "ID:HEXDATA" (e.g. "1280:AABBCC...")
        try {
            size_t colonPos = msg.find(':');
            if (colonPos == std::string::npos) return;

            std::string idStr = msg.substr(0, colonPos);
            std::string dataStr = msg.substr(colonPos + 1);
            
            uint32_t id = std::stoul(idStr);
            if (id != recvId_) return; // Filter by expected ID

            // Hex string to bytes
            std::vector<uint8_t> data;
            for (size_t i = 0; i < dataStr.length(); i += 2) {
                std::string byteString = dataStr.substr(i, 2);
                data.push_back((uint8_t)strtol(byteString.c_str(), nullptr, 16));
            }

            if (data.size() < 8) return;

            // Parse bytes: 0:Status, 1:Res, 2-3:Cur, 4-5:Spd, 6-7:Ang
            // Little Endian
            int16_t rawCurrent = (int16_t)(data[2] | (data[3] << 8));
            int16_t rawSpeed   = (int16_t)(data[4] | (data[5] << 8));
            int16_t rawAngle   = (int16_t)(data[6] | (data[7] << 8));

            MotorState newState;
            newState.status = data[0];
            newState.current = ctrlValueToCurrent(rawCurrent);
            newState.speed = ctrlValueToSpeed(rawSpeed);
            newState.angle = ctrlValueToAngle(rawAngle);

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
            auto elapsed = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();
            if (elapsed < 1000) {
                std::this_thread::sleep_for(std::chrono::microseconds(1000 - elapsed));
            }
        }
    }
};

// Main
int main(int, char**) {
    glfwSetErrorCallback(glfw_error_callback);
    if (!glfwInit()) return 1;

    const char* glsl_version = "#version 130";
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);

    GLFWwindow* window = glfwCreateWindow(1280, 720, "QDrive Motor Tuner", NULL, NULL);
    if (window == NULL) return 1;
    glfwMakeContextCurrent(window);
    glfwSwapInterval(1); // Enable vsync

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    ImGui::StyleColorsDark();

    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init(glsl_version);

    // App state
    MotorController motor;
    bool connected = false;
    char serialDev[64] = "/dev/ttyACM0";
    char canIf[64] = "can0";
    
    // Control vars
    float targetSpeed = 0.0f;
    float targetAngle = 0.0f;
    float targetCurrent = 0.0f;
    int controlMode = 0; // 0: None/Status, 1: Speed, 2: Angle, 3: Current

    // ID config
    int sendIdInput = 0x400;
    int recvIdInput = 0x500;

    // PID vars
    float speedKP=0, speedKI=0, speedKD=0;
    float angleKP=0, angleKI=0, angleKD=0;

    float limitSpeed = 1000.0f;
    float limitCurrent = 10.0f;
    int configCanId = 0;
    int configBaud = 115200;

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
            ImGui::InputInt("Send ID (Hex)", &sendIdInput, 1, 100, ImGuiInputTextFlags_CharsHexadecimal);
            ImGui::InputInt("Recv ID (Hex)", &recvIdInput, 1, 100, ImGuiInputTextFlags_CharsHexadecimal);

            if (ImGui::Button("Connect")) {
                motor.setCanIds(static_cast<uint32_t>(sendIdInput), static_cast<uint32_t>(recvIdInput));
                if (motor.init(serialDev, canIf)) {
                    connected = true;
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

        ImGui::PlotLines("Speed (rpm)", speedHistory.data(), speedHistory.size(), 0, NULL, -1000, 1000, ImVec2(0, 80));
        ImGui::PlotLines("Angle (rad)", angleHistory.data(), angleHistory.size(), 0, NULL, 0, PI*2, ImVec2(0, 80));
        ImGui::PlotLines("Current (A)", currentHistory.data(), currentHistory.size(), 0, NULL, -10, 10, ImVec2(0, 80));

        ImGui::Separator();

        // Control
        if (connected) {
            if (ImGui::Button("Enable")) motor.enable();
            ImGui::SameLine();
            if (ImGui::Button("Disable")) motor.disable();
            ImGui::SameLine();
            if (ImGui::Button("Silent")) motor.silent();
            ImGui::SameLine();
            if (ImGui::Button("Reboot")) motor.reboot();

            ImGui::RadioButton("Monitor", &controlMode, 0); ImGui::SameLine();
            ImGui::RadioButton("Speed Control", &controlMode, 1); ImGui::SameLine();
            ImGui::RadioButton("Angle Control", &controlMode, 2); ImGui::SameLine();
            ImGui::RadioButton("Current Control", &controlMode, 3);

            if (controlMode == 1) {
                if (ImGui::SliderFloat("Target Speed", &targetSpeed, MIN_SPEED_CTRL_VALUE, MAX_SPEED_CTRL_VALUE)) {
                    motor.setTargetSpeed(targetSpeed);
                }
            } else if (controlMode == 2) {
                if (ImGui::SliderFloat("Target Angle", &targetAngle, MIN_ANGLE_CTRL_VALUE, MAX_ANGLE_CTRL_VALUE)) {
                    motor.setTargetAngle(targetAngle);
                }
            } else if (controlMode == 3) {
                if (ImGui::SliderFloat("Target Current", &targetCurrent, MIN_CURRENT_CTRL_VALUE, MAX_CURRENT_CTRL_VALUE)) {
                    motor.setTargetCurrent(targetCurrent);
                }
            } else {
                 // Monitor mode send no control loop commands except status in background
            }
        }

        ImGui::Separator();
        
        // Configuration (Serial)
        if (ImGui::CollapsingHeader("PID Configuration")) {
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
            if(ImGui::Button("Set Speed Limit")) motor.configLimitSpeed(limitSpeed);
            
            ImGui::InputFloat("Limit Current", &limitCurrent);
            if(ImGui::Button("Set Current Limit")) motor.configLimitCurrent(limitCurrent);

            ImGui::InputInt("New CAN ID", &configCanId);
            if(ImGui::Button("Set CAN ID (0-8)")) motor.configCanID((uint32_t)configCanId);

            ImGui::InputInt("Baud Rate", &configBaud);
            if(ImGui::Button("Set Baud Rate")) motor.configBaudRate((uint32_t)configBaud);
        }

        if (ImGui::Button("Store Config")) motor.store();
        ImGui::SameLine();
        if (ImGui::Button("Restore Config")) motor.restore();
        
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

#include <iostream>

#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include <GLFW/glfw3.h>

#include "app_config.hpp"
#include "motor_controller.hpp"

#include <algorithm>
#include <memory>

// Forward declaration of helper
static void glfw_error_callback(int error, const char* description) {
    fprintf(stderr, "Glfw Error %d: %s\n", error, description);
}

static void PlotLinesWithGrid(
    const char* label,
    const float* values,
    int values_count,
    float scale_min,
    float scale_max,
    ImVec2 size        = ImVec2(0, 80),
    int grid_x         = 10,
    int grid_y         = 5,
    float target_value = NAN // Optional target value to draw as reference line
) {
    if (values_count <= 0)
        return;
    if (size.x == 0.0f)
        size.x = ImGui::GetContentRegionAvail().x - 100;

    ImVec2 pos = ImGui::GetCursorScreenPos();
    ImVec2 br(pos.x + size.x, pos.y + size.y);
    ImDrawList* dl = ImGui::GetWindowDrawList();

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

    // Draw target reference line if provided and within range
    if (!std::isnan(target_value) && target_value >= scale_min && target_value <= scale_max) {
        float t         = (target_value - scale_min) / (scale_max - scale_min);
        float target_y  = pos.y + size.y * (1.0f - t);
        ImU32 targetCol = ImGui::GetColorU32(ImVec4(1.0f, 0.2f, 0.2f, 0.8f)); // Red

        // Draw dashed line
        float dash_length = 8.0f;
        float gap_length  = 4.0f;
        for (float x = pos.x; x < br.x; x += dash_length + gap_length) {
            float x_end = std::min(x + dash_length, br.x);
            dl->AddLine(ImVec2(x, target_y), ImVec2(x_end, target_y), targetCol, 2.0f);
        }
    }
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
    glfwSwapInterval(1);

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    (void)io;
    ImGui::StyleColorsDark();

    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init(glsl_version);

    // App state
    auto motor     = std::make_unique<MotorController>();
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
    int controlMode     = 0;

    // ID config
    int sendIdInput = appCfg.sendIdHex;
    int recvIdInput = appCfg.recvIdHex;

    // PID vars
    float speedKP = 0, speedKI = 0, speedKD = 0;
    float angleKP = 0, angleKI = 0, angleKD = 0;

    float limitSpeed   = 1000.0f;
    float limitCurrent = 3.0f;

    // Display-only ranges (do NOT affect control calculations). Initialize from persisted config
    float displayMinSpeed   = appCfg.displayMinSpeed;
    float displayMaxSpeed   = appCfg.displayMaxSpeed;
    float displayMinAngle   = appCfg.displayMinAngle;
    float displayMaxAngle   = appCfg.displayMaxAngle;
    float displayMinCurrent = appCfg.displayMinCurrent;
    float displayMaxCurrent = appCfg.displayMaxCurrent;

    int configCanId = 0;
    int configBaud  = 115200;

    // Plotting buffers
    std::vector<float> speedHistory(1000, 0);
    std::vector<float> angleHistory(1000, 0);
    std::vector<float> currentHistory(1000, 0);
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
                motor->setCanIds(
                    static_cast<uint32_t>(sendIdInput),
                    static_cast<uint32_t>(recvIdInput)
                );
                if (motor->init(serialDev, canIf)) {
                    connected           = true;
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
                motor->stop();
                motor.reset(new MotorController());
                connected = false;

                std::fill(speedHistory.begin(), speedHistory.end(), 0.0f);
                std::fill(angleHistory.begin(), angleHistory.end(), 0.0f);
                std::fill(currentHistory.begin(), currentHistory.end(), 0.0f);
            }
        }

        ImGui::Separator();

        // Status view
        MotorState state = motor->getState();
        ImGui::Text("Status: %d", state.status);
        ImGui::Text("Speed: %.6f rpm", state.speed);
        ImGui::Text("Angle: %.6f rad", state.angle);
        ImGui::Text("Current: %.6f A", state.current);

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
            displayMinSpeed,
            displayMaxSpeed,
            ImVec2(0, 160),
            10,
            5,
            controlMode == 1 ? targetSpeed : NAN // Show target in speed control mode
        );
        PlotLinesWithGrid(
            "Angle (rad)",
            angleHistory.data(),
            angleHistory.size(),
            displayMinAngle,
            displayMaxAngle,
            ImVec2(0, 160),
            10,
            5,
            controlMode == 2 ? targetAngle : NAN // Show target in angle control mode
        );
        PlotLinesWithGrid(
            "Current (A)",
            currentHistory.data(),
            currentHistory.size(),
            displayMinCurrent,
            displayMaxCurrent,
            ImVec2(0, 160),
            10,
            5,
            controlMode == 3 ? targetCurrent : NAN // Show target in current control mode
        );

        ImGui::Separator();

        // Control
        if (connected) {
            if (ImGui::Button("Enable"))
                motor->enable();
            ImGui::SameLine();
            if (ImGui::Button("Disable"))
                motor->disable();
            ImGui::SameLine();
            if (ImGui::Button("Reboot"))
                motor->reboot();

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
                    displayMinSpeed,
                    displayMaxSpeed
                );
                ImGui::SameLine();
                bool inputChanged =
                    ImGui::InputFloat("##SpeedInput", &targetSpeed, 1.0f, 10.0f, "%.2f");
                if (inputChanged) {
                    targetSpeed =
                        std::clamp(targetSpeed, MIN_SPEED_CTRL_VALUE, MAX_SPEED_CTRL_VALUE);
                }
                if (sliderChanged || inputChanged) {
                    motor->setTargetSpeed(targetSpeed);
                }
            } else if (controlMode == 2) {
                bool sliderChanged = ImGui::SliderFloat(
                    "Target Angle",
                    &targetAngle,
                    displayMinAngle,
                    displayMaxAngle
                );
                ImGui::SameLine();
                bool inputChanged =
                    ImGui::InputFloat("##AngleInput", &targetAngle, 0.1f, 1.0f, "%.3f");
                if (inputChanged) {
                    targetAngle =
                        std::clamp(targetAngle, MIN_ANGLE_CTRL_VALUE, MAX_ANGLE_CTRL_VALUE);
                }
                if (sliderChanged || inputChanged) {
                    motor->setTargetAngle(targetAngle);
                }
            } else if (controlMode == 3) {
                bool sliderChanged = ImGui::SliderFloat(
                    "Target Current",
                    &targetCurrent,
                    displayMinCurrent,
                    displayMaxCurrent
                );
                ImGui::SameLine();
                bool inputChanged =
                    ImGui::InputFloat("##CurrentInput", &targetCurrent, 0.1f, 1.0f, "%.2f");
                if (inputChanged) {
                    targetCurrent =
                        std::clamp(targetCurrent, MIN_CURRENT_CTRL_VALUE, MAX_CURRENT_CTRL_VALUE);
                }
                if (sliderChanged || inputChanged) {
                    motor->setTargetCurrent(targetCurrent);
                }
            } else {
                // Monitor mode
            }
        }

        ImGui::Separator();

        // Configuration (Serial)
        if (ImGui::CollapsingHeader("PID Configuration")) {
            if (ImGui::Button("Refresh Config")) {
                motor->requestConfigRefresh();
            }

            ConfigSnapshot cfg = motor->getConfigSnapshot();
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
                ConfigSnapshot snap = motor->getConfigSnapshot();
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
                motor->configSpeedPID(speedKP, speedKI, speedKD);
                motor->configAnglePID(angleKP, angleKI, angleKD);
                motor->configLimitSpeed(limitSpeed);
                motor->configLimitCurrent(limitCurrent);
                motor->configCanID((uint32_t)configCanId);
                motor->configBaudRate((uint32_t)configBaud);
                std::cout << "[ConfigSend] Sent config to motor." << std::endl;
            }

            ImGui::Text("Speed PID");
            ImGui::InputFloat("S-KP", &speedKP, 0.0f, 0.0f, "%.6f");
            ImGui::InputFloat("S-KI", &speedKI, 0.0f, 0.0f, "%.6f");
            ImGui::InputFloat("S-KD", &speedKD, 0.0f, 0.0f, "%.6f");
            if (ImGui::Button("Apply Speed PID")) {
                motor->configSpeedPID(speedKP, speedKI, speedKD);
            }

            ImGui::Separator();
            ImGui::Text("Angle PID");
            ImGui::InputFloat("A-KP", &angleKP, 0.0f, 0.0f, "%.6f");
            ImGui::InputFloat("A-KI", &angleKI, 0.0f, 0.0f, "%.6f");
            ImGui::InputFloat("A-KD", &angleKD, 0.0f, 0.0f, "%.6f");
            if (ImGui::Button("Apply Angle PID")) {
                motor->configAnglePID(angleKP, angleKI, angleKD);
            }

            ImGui::Separator();
            ImGui::Text("Limits & Sys Config");
            ImGui::InputFloat("Limit Speed", &limitSpeed);
            if (ImGui::Button("Set Speed Limit"))
                motor->configLimitSpeed(limitSpeed);

            ImGui::InputFloat("Limit Current", &limitCurrent);
            if (ImGui::Button("Set Current Limit"))
                motor->configLimitCurrent(limitCurrent);

            // Display ranges moved to their own collapsing header below
            ImGui::Separator();

            ImGui::InputInt("New CAN ID", &configCanId);
            if (ImGui::Button("Set CAN ID (0-8)"))
                motor->configCanID((uint32_t)configCanId);

            ImGui::InputInt("Baud Rate", &configBaud);
            if (ImGui::Button("Set Baud Rate"))
                motor->configBaudRate((uint32_t)configBaud);

            ImGui::Separator();
            if (ImGui::Button("Store Config"))
                motor->store();
            ImGui::SameLine();
            if (ImGui::Button("Restore Config"))
                motor->restore();
        }

        if (ImGui::CollapsingHeader("Display Ranges")) {
            ImGui::InputFloat("Display Min Speed", &displayMinSpeed);
            ImGui::InputFloat("Display Max Speed", &displayMaxSpeed);
            ImGui::InputFloat("Display Min Angle", &displayMinAngle);
            ImGui::InputFloat("Display Max Angle", &displayMaxAngle);
            ImGui::InputFloat("Display Min Current", &displayMinCurrent);
            ImGui::InputFloat("Display Max Current", &displayMaxCurrent);

            // Simple guards to keep display ranges valid
            if (displayMinSpeed >= displayMaxSpeed)
                displayMaxSpeed = displayMinSpeed + 1e-3f;
            if (displayMinAngle >= displayMaxAngle)
                displayMaxAngle = displayMinAngle + 1e-6f;
            if (displayMinCurrent >= displayMaxCurrent)
                displayMaxCurrent = displayMinCurrent + 1e-3f;

            if (ImGui::Button("Save Display Ranges")) {
                appCfg.displayMinSpeed   = displayMinSpeed;
                appCfg.displayMaxSpeed   = displayMaxSpeed;
                appCfg.displayMinAngle   = displayMinAngle;
                appCfg.displayMaxAngle   = displayMaxAngle;
                appCfg.displayMinCurrent = displayMinCurrent;
                appCfg.displayMaxCurrent = displayMaxCurrent;
                saveConfig(appCfg);
                std::cout << "[Config] Saved display ranges to qdrive_gui.conf" << std::endl;
            }
        }

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

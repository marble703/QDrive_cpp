#pragma once

#include "base/interfacebase.hpp"

#include <boost/numeric/conversion/cast.hpp>

namespace qdriver::interface {

enum class PIDtype { KP, KI, KD };

#if __cplusplus >= 202002L
constexpr float PI = std::numbers::pi_v<float>; // c++20
#else
constexpr float PI = 3.14159265358979323846f * 2;
#endif

static const float MAX_CURRENT_CTRL_VALUE   = 10;
static const float MIN_CURRENT_CTRL_VALUE   = -10;
static const float MAX_SPEED_CTRL_VALUE     = 1000;
static const float MIN_SPEED_CTRL_VALUE     = -1000;
static const float MAX_ANGLE_CTRL_VALUE     = PI * 2;
static const float MIN_ANGLE_CTRL_VALUE     = 0;
static const float MAX_STEPANGLE_CTRL_VALUE = PI * 5;
static const float MIN_STEPANGLE_CTRL_VALUE = PI * -5;
class Interface: public InterfaceBase {
public:
    Interface(std::shared_ptr<qdriver::io::Serial> serialPort);
    Interface(std::shared_ptr<qdriver::io::Can> canPort, uint32_t defalutSendCanID = -1);

    ~Interface() = default;

    // 查询指令
    /**
     * @brief 显示 QD4310 命令帮助信息
     * 
     * @pre 仅对串口接口有效
     */
    bool help();

    /**
     * @brief 显示电机硬件和软件版本信息
     * 
     * @pre 仅对串口接口有效
     */
    bool version();

    /**
     * @brief 显示极对数、相电阻、相电感等电机固有信息
     * 
     * @pre 仅对串口接口有效
     */
    bool info();

    /**
     * @brief 显示电压、电流、转速、控制模式等运行状态
     */
    bool status(int canID = -1);

    // 控制指令

    /**
     * @brief 使能电机
     * @note 注意未做基础校准无法使能
     */
    bool enable(int canID = -1);

    /**
     * @brief 失能电机
     * @note 电机失能后控制状态一直保留，重新使能恢复原有控制状态
     */
    bool disable(int canID = -1);

    /**
     * @brief 静默输出
     * @note 静默电机返回数据，以降低串口带宽占用，提高控制频率
     * 
     * @pre 仅对串口接口有效
     */
    bool silent();

    /**
    * @brief 重启电机
    * @note 注意重启后上位机需重新连接
    * 
    * @pre 仅对串口接口有效
    */
    bool reboot();

    /**
     * @brief 电流控制模式
     * @note Q 轴电流正方向（逆时针）
     * @note QD4310 的扭矩常数为 0.3Nm/A
     *
     * @param current 单位 A
     */
    bool ctrlCurrent(float current, int canID = -1);

    /**
     * @brief 速度控制模式
     * @note 沿正方向（逆时针）
     *
     * @param speed 单位 rpm
     */
    bool ctrlSpeed(float speed, int canID = -1);

    /**
     * @brief 角度控制模式
     * @note 沿正方向（逆时针）
     *
     * @param angle 单位 rad
     */
    bool ctrlAngle(float angle, int canID = -1);

    /**
    * @brief 低速控制模式
    * @note 沿正方向（逆时针）
    *
    * @param speed 单位 rpm
    */
    bool ctrlLowSpeed(float speed, int canID = -1);

    /**
     * @brief 角度步进控制模式
     * @note 电机将会以 limit.speed 的速度上限转动（前提是能达到此速度）
     *
     * @param angle 单位 rad
     */
    bool ctrlStepAngle(float angle, int canID = -1);

    /**
     * @brief 储存当前配置参数
     * 
     * @pre 仅对串口接口有效
     */
    bool store();

    /**
     * @brief 恢复出厂设置
     * 
     * @pre 仅对串口接口有效
     */
    bool restore();

    /**
     * @brief 配置速度控制的 PID 参数
     * 
     * @param value 参数值
     * @param pidType 参数类型
     */
    bool configSpeed(float value, PIDtype pidType);

    /**
     * @brief 配置角度控制的 PID 参数
     * 
     * @param value 参数值
     * @param pidType 参数类型
     */
    bool configAngle(float value, PIDtype pidType);

    /**
     * @brief 配置速度限幅
     * 
     * @param speed 单位 rpm
     */
    bool configLimitSpeed(float speed);

    /**
     * @brief 配置电流限幅
     * 
     * @param current 单位 A
     */
    bool configLimitCurrent(float current);

    /**
     * @brief 配置 CAN ID
     * 
     * @param canID CAN ID 限制 0 - 8
     */
    bool configCanID(uint32_t canID);

    /**
     * @brief 配置串口波特率
     * 
     * @param baudRate 波特率
     */
    bool configBaudRate(unsigned int baudRate);

private:
    uint32_t externalCanID(int canID) const;

    // 将电流 -10A ~ 10A 映射到 int16 的 -32768 ~ 32767
    int16_t curentToCtrlValue(float current) const;

    // 将速度 -1000rpm ~ 1000rpm 映射到 int16 的 -32768 ~ 32767
    int16_t speedToCtrlValue(float speed) const;

    // 将速度 0 ~ 2pi 映射到 int16 的 -32768 ~ 32767
    int16_t angleToCtrlValue(float angle) const;

    // 将速度 -5pi ~ 5pi 映射到 int16 的 -32768 ~ 32767
    int16_t stepAngleToCtrlValue(float angle) const;

    int defalutSendCanID { -1 };
};

} // namespace qdriver::interface

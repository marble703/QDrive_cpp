#include "interface.hpp"

namespace qdriver::motor {

using qdriver::interface::ioType;
using qdriver::interface::PIDtype;

class Motor {
public:
    Motor(
        std::shared_ptr<qdriver::interface::Interface> interfacePtr,
        const std::string& name = "defaultmotor",
        size_t sendCanID        = 0x400,
        size_t receiveCanID     = 0x500
    );

    bool addInterface(std::shared_ptr<qdriver::interface::Interface> interfacePtr);

    bool removeInterface(const ioType);

    std::shared_ptr<qdriver::interface::Interface> getInterface(const ioType ioType) const;

    bool setCanID(size_t sendCanID, size_t receiveCanID);

    bool getCanID(size_t& sendCanID, size_t& receiveCanID) const;

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
    bool status(ioType ioType = ioType::NONE);

    // 控制指令

    /**
     * @brief 使能电机
     * @note 注意未做基础校准无法使能
     */
    bool enable(ioType ioType = ioType::NONE);

    /**
     * @brief 失能电机
     * @note 电机失能后控制状态一直保留，重新使能恢复原有控制状态
     */
    bool disable(ioType ioType = ioType::NONE);

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
    bool ctrlCurrent(float current, ioType ioType = ioType::NONE);

    /**
     * @brief 速度控制模式
     * @note 沿正方向（逆时针）
     *
     * @param speed 单位 rpm
     */
    bool ctrlSpeed(float speed, ioType ioType = ioType::NONE);

    /**
     * @brief 角度控制模式
     * @note 沿正方向（逆时针）
     *
     * @param angle 单位 rad
     */
    bool ctrlAngle(float angle, ioType ioType = ioType::NONE);

    /**
    * @brief 低速控制模式
    * @note 沿正方向（逆时针）
    *
    * @param speed 单位 rpm
    */
    bool ctrlLowSpeed(float speed, ioType ioType = ioType::NONE);

    /**
     * @brief 角度步进控制模式
     * @note 电机将会以 limit.speed 的速度上限转动（前提是能达到此速度）
     *
     * @param angle 单位 rad
     */
    bool ctrlStepAngle(float angle, ioType ioType = ioType::NONE);

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

    std::string name_;
private:
    size_t sendCanID_;
    size_t receiveCanID_;

    std::array<std::shared_ptr<qdriver::interface::Interface>, 2> interfaces_;
};
} // namespace qdriver::motor
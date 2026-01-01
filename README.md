# QDriver SDK (C++)

## 简介

本项目为[QDriver](https://github.com/Liu-Curiousity/QDrive-Software)上位机 cpp 开发工具

正在开发中，欢迎提 issue 和 pr

## 开发进度

- [x] 串口通信
- [x] can 通信
- [x] 接口
- [ ] 日志
- [ ] 调试工具
- [ ] Mock

## 代码结构

io(通信功能封装) 
-> interfacebase(基础通信协议封装) 
-> interface(高级通信协议封装)

## 开发环境

使用 QD4310 电机进行开发和测试

Ubuntu 22.04 
g++ 11.4.0 
boost 1.74

Nyarch Linux

## 依赖

建议使用 `g++-10.1`, `Boost 1.70` 以上版本

## 编译

`cmake -S . -B build && cmake --build build -j`

## 运行示例

**可能包含电机使能，电机旋转等操作，注意运行风险**

串口：

单个电机
`./build/example/qdrive_example`

多个
`./build/example/qdrive_example_two`

终端输出
`./build/example/qdrive_example_reader`

can：
单个电机
`./build/example/qdrive_example_can`
或是

终端输出
`./build/example/qdrive_example_reader_can`
或是使用 `candump`



## usb-rule

设置 usb rule

``` sh
chmod +x create-udev-rule.sh
./script/create-udev-rule.sh /dev/ttyACM0 QD4310-0 
```

## 启动 can 通信

```sh
sudo ip link set can0 up type can bitrate 1000000
```

## 开源声明

本项目使用 [**GNU General Public License v3.0**](https://www.gnu.org/licenses/gpl-3.0.en.html) 许可证

Copyright (C) 2025 marble703

本项目与[QDriver](https://github.com/Liu-Curiousity/QDrive-Software)通过 can/serial 协议进行通信
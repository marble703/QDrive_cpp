# QDriver SDK (C++)

[QDriver](https://github.com/Liu-Curiousity/QDrive-Software)上位机 cpp 开发工具

使用 QD4310 电机进行开发和测试

## 开发进度

- [x] 串口通信
- [ ] can 通信
- [ ] 接口(部分完成)
- [ ] 日志
- [ ] 调试工具
- [ ] Mock

当前进度：

发现 serial 在 50 HZ 及以上控制频率疑似出现硬件缓存溢出

等 usb-can 线

## 代码结构

io(通信功能封装) 
-> interfacebase(基础通信协议封装) 
-> interface(高级协议封装)

## 开发环境

Ubuntu 22.04 
g++ 11.4.0 
boost 1.74

Nyarch Linux

## 依赖

使用了 `boost` 库的 `boost::asio::serial_port`

`C++17` 的 `filesystem`

建议使用 `g++-9.3`, `Boost 1.70` 以上版本

## 编译

`cmake -S . -B build && cmake --build build -j`

## 运行示例


**当前示例并不完善**

**且可能包含了包括但不限于: 电机使能， 电机旋转等危险操作**

单个电机
`./build/example/qdrive_example`

多个
`./build/example/qdrive_example_two`

终端输出
`./build/example/qdrive_example_reader`

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

本项目暂时处于未发布状态

本项目使用 [**GNU General Public License v3.0**](https://www.gnu.org/licenses/gpl-3.0.en.html) 许可证

Copyright (C) 2025 marble703

本项目与[QDriver](https://github.com/Liu-Curiousity/QDrive-Software)通过 can/serial 协议进行通信
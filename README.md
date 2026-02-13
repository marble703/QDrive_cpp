# QDrive SDK (C++)

[![Build on Linux](https://github.com/marble703/QDrive_cpp/actions/workflows/build-linux.yml/badge.svg)](https://github.com/marble703/QDrive_cpp/actions/workflows/build-linux.yml)
[![Build on Windows](https://github.com/marble703/QDrive_cpp/actions/workflows/build-windows.yml/badge.svg)](https://github.com/marble703/QDrive_cpp/actions/workflows/build-windows.yml)

## 简介

本项目为[QDrive](https://github.com/Liu-Curiousity/QDrive-Software)上位机 cpp 开发工具

正在开发中，欢迎提 issue 和 pr

windows 支持开发中

已知问题： 在 gui 中，未启动 can 可能导致程序崩溃 
点击 disconnect 可能导致程序崩溃

## 代码结构

Driver(驱动部分) 
-> io(通信功能封装) 
-> interfacebase(基础通信协议封装) 
-> interface(高级通信协议封装) 
-> motor(电机功能封装) 
logger(日志功能封装)

## 开发环境

使用 QD4310 电机进行开发和测试

Ubuntu 22.04 
g++ 11.4.0 
boost 1.74

Arch Linux

## 依赖

本项目依赖:

通信基础层：
* [Boost](https://www.boost.org/) (BSL-1.0 License)

日志模块：
* [spdlog](https://github.com/gabime/spdlog) (MIT License)

图形化界面：
* [imgui](https://github.com/ocornut/imgui) (MIT License)

## 拉取和构建

(如果需要调试界面可能需要手动安装 glfw3, 日志库可能需要手动安装 fmt)

`sudo apt install libboost-dev libfmt-dev pkg-config`

` git submodule update --init --recursive`

` cmake -S . -B build -DCMAKE_BUILD_TYPE=Release && cmake --build build -j`

可以设置 `WITHOUT_GUI=ON` 参数忽略调试界面及其 imgui 依赖 
可以设置 `WITHOUT_LOGGER=ON` 参数忽略日志模块及其 spdlog 依赖 
由于 example 依赖 logger 依赖，会同时忽略 example 部分

### Windows 编译(SLCAN)

- 推荐工具链：Visual Studio 2022（"桌面开发（使用 C++）" 工作负载） + CMake。
- 我们在 CMake 中已为 MSVC 自动添加 `/utf-8` 和 `_WIN32_WINNT=0x0601`（无需手动设置）。

必装 vcpkg 库：
```bat
vcpkg install boost-headers boost-asio fmt
```

在项目根目录构建：
```bat
cmake -S . -B build -G "Visual Studio 17 2022" -A x64 -DWITHOUT_GUI=ON -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_TOOLCHAIN_FILE=C:/path/to/vcpkg/scripts/buildsystems/vcpkg.cmake
cmake --build build --config Release
```

- 运行 CAN 示例时，请传入 SLCAN 风格的串口端点，例如 `COM3@500000` 或 `slcan:COM3@500000`。
  - 支持的标准波特率（SLCAN）：10000/20000/50000/100000/125000/250000/500000/750000/1000000。
  - 当前 Windows 后端实验性实现支持标准 11-bit 帧

#### CI/CD 自动测试

本项目使用 GitHub Actions 自动编译测试，覆盖：

- **Linux (Ubuntu 22.04)**：完整构建（含界面）、 `WITHOUT_GUI`、`WITHOUT_LOGGER` 变体
- **Windows (Latest)**：MSVC + vcpkg 完整构建（`WITHOUT_GUI=ON`）

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

## 脚本

设置 usb-rule 
需要为脚本添加执行权限, 传入设备路径 和 设备名称

``` sh
chmod +x create-udev-rule.sh
./script/create-udev-rule.sh /dev/ttyACM0 QD4310-0 
```

## 工具

调试界面

![](image/README/gui.png)

`./build/tool/gui/qdrive_gui`

串口调试工具

`./build/tool/io/qdrive_tool_serial`

CAN调试工具

`./build/tool/io/qdrive_tool_can`

## 小指令

启动 can 通信

```sh
sudo ip link set can0 up type can bitrate 1000000
```

搜索 can 设备

```sh
ip link show | grep can
```

发送/接收 can 数据

```sh
cansend can0 400#000000
candump can0
```

## 开源声明

本项目使用 [**GNU General Public License v3.0**](https://www.gnu.org/licenses/gpl-3.0.en.html) 许可证

Copyright (C) 2025 marble703

本项目与[QDriver](https://github.com/Liu-Curiousity/QDrive-Software)通过 can/serial 协议进行通信
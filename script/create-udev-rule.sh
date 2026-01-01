#!/bin/bash

if [ $# -ne 2 ]; then
    echo "Usage: $0 <device_path> <name>"
    exit 1
fi

DEV_PATH="$1"
NAME="$2"

# 获取设备的 idVendor, idProduct, serial（取前3行）
OUTPUT=$(udevadm info -a -n "$DEV_PATH" | grep -E 'idVendor|idProduct|serial')

if [ -z "$OUTPUT" ]; then
    echo "Error: Failed to retrieve device attributes."
    exit 1
fi

# 提取值
ID_VENDOR=$(echo "$OUTPUT" | grep idVendor | sed -n 's/.*== "\(.*\)".*/\1/p')
ID_PRODUCT=$(echo "$OUTPUT" | grep idProduct | sed -n 's/.*== "\(.*\)".*/\1/p')
SERIAL=$(echo "$OUTPUT" | grep serial | sed -n 's/.*== "\(.*\)".*/\1/p')

if [ -z "$ID_VENDOR" ] || [ -z "$ID_PRODUCT" ] || [ -z "$SERIAL" ]; then
    echo "Error: Missing required attributes (idVendor, idProduct, or serial)."
    exit 1
fi

# 生成规则
RULE="SUBSYSTEM==\"tty\", ATTRS{idVendor}==\"$ID_VENDOR\", ATTRS{idProduct}==\"$ID_PRODUCT\", ATTRS{serial}==\"$SERIAL\", SYMLINK+=\"$NAME\""

# 写入规则文件
echo "$RULE" | sudo tee /etc/udev/rules.d/99-usb-serial.rules > /dev/null

echo "udev rules loaded, please unplug and replug the device to take effect."

sudo udevadm control --reload-rules            
sudo udevadm trigger

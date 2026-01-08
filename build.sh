#!/bin/bash
# AccountSvr 编译脚本 (Linux/MacOS版本)
# 此脚本会在 build 目录中生成项目

echo "========================================"
echo "          AccountSvr 编译脚本"
echo "========================================"
echo

# 检查是否存在CMakeLists.txt
if [ ! -f "CMakeLists.txt" ]; then
    echo "错误: 未找到 CMakeLists.txt 文件"
    echo "请确保在项目根目录中运行此脚本"
    exit 1
fi

echo "[1/3] 清理旧的构建文件..."
if [ -d "build" ]; then
    echo "删除旧的 build 目录..."
    rm -rf "build"
fi

echo "[2/3] 创建新的构建目录..."
mkdir "build"
cd "build"

echo "[3/3] 配置并编译项目..."
echo "运行 CMake 配置..."

# 配置CMake项目
cmake ..

if [ $? -ne 0 ]; then
    echo "CMake 配置失败！"
    cd ..
    exit 1
fi

echo
echo "开始编译项目..."
make -j$(nproc)

if [ $? -ne 0 ]; then
    echo "编译失败！"
    cd ..
    exit 1
fi

echo
echo "========================================"
echo "           编译成功完成！"
echo "========================================"
echo "可执行文件位置: build/AccountSvr"
echo

cd ..
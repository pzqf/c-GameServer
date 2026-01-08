# AccountSvr 编译环境设置指南

## 错误说明

你遇到的错误：
```
The CXX compiler identification is unknown
No CMAKE_CXX_COMPILER could be found
```

这个错误表示系统无法找到C++编译器，需要正确配置编译环境。

## Windows 编译环境设置

### 方法1: 使用Visual Studio (推荐)

1. **安装 Visual Studio 2022**
   - 下载 Visual Studio 2022 Community (免费)
   - 安装时选择"Desktop development with C++"工作负载

2. **使用 Developer Command Prompt**
   - 打开 "Developer Command Prompt for VS 2022"
   - 在此命令提示符中运行 `build.bat` 脚本

3. **或者使用 Visual Studio 开发者控制台**
   - 打开 Visual Studio 2022
   - 菜单: Tools → Command Line → Developer Command Prompt

### 方法2: 使用 Visual Studio Build Tools

如果只需要编译器而不需要完整的IDE：

1. **下载 Visual Studio Build Tools**
   - 下载 "Build Tools for Visual Studio"
   - 选择 "C++ build tools" 工作负载

2. **使用内置的命令行工具**
   - 安装后会有 "x64 Native Tools Command Prompt for VS 2022"

### 方法3: 使用 MSYS2 + MinGW

1. **安装 MSYS2**
   - 下载并安装 MSYS2
   - 打开 MSYS2 MINGW64 终端

2. **安装编译工具链**
   ```bash
   pacman -S mingw-w64-x86_64-gcc
   pacman -S mingw-w64-x86_64-cmake
   pacman -S mingw-w64-x86_64-make
   ```

3. **使用MinGW编译**
   ```bash
   cd /d/d/work/AccountSvr
   mkdir build && cd build
   cmake .. -G "MinGW Makefiles"
   make
   ```

## 编译脚本使用

### Windows (build.bat)

```bash
# 确保在 Developer Command Prompt 中运行
build.bat
```

### Linux/macOS (build.sh)

```bash
chmod +x build.sh
./build.sh
```

## 验证安装

运行以下命令验证环境是否正确配置：

```bash
# 检查 cmake
cmake --version

# 检查编译器 (Windows)
cl

# 检查编译器 (Linux/macOS)
g++ --version
```

## 故障排除

1. **"cmake不是内部或外部命令"**
   - 安装 CMake 并添加到 PATH 环境变量
   - 重启命令提示符

2. **"找不到编译器"**
   - 确保使用 Developer Command Prompt
   - 检查 Visual Studio 安装是否完整

3. **权限错误**
   - 以管理员身份运行命令提示符
   - 检查文件夹访问权限

## 项目构建输出

成功编译后，可执行文件将位于：
- **Windows**: `build/Release/AccountSvr.exe`
- **Linux/macOS**: `build/AccountSvr`

## 注意事项

- 始终在正确的开发环境中运行编译脚本
- 如果修改了编译器设置，记得清理build目录重新构建
- 建议使用 Release 模式进行发布构建
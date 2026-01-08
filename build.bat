@echo off
REM AccountSvr Simple Build Script
REM This script builds the project in the build directory

echo ========================================
echo        AccountSvr Build Script
echo ========================================
echo.

REM Check if CMakeLists.txt exists
if not exist "CMakeLists.txt" (
    echo Error: CMakeLists.txt not found
    echo Please ensure this script is run from the project root directory
    pause
    exit /b 1
)

echo [Step 1/3] Cleaning old build files...
if exist "build" (
    echo Removing old build directory...
    rmdir /s /q "build" 2>nul
)

echo [Step 2/3] Creating new build directory...
mkdir "build"
cd "build"

echo [Step 3/3] Configuring and building project...
echo Running CMake configuration...

REM Try different CMake generators based on available tools
cmake .. > cmake_output.log 2>&1
if %errorlevel% neq 0 (
    echo CMake configuration failed!
    echo Check cmake_output.log for details
    echo.
    echo Common solutions:
    echo 1. Install Visual Studio or Build Tools
    echo 2. Use Developer Command Prompt for VS
    echo 3. Install MinGW and use -G "MinGW Makefiles"
    cd ..
    type cmake_output.log
    pause
    exit /b 1
)

echo Starting project build...
cmake --build . --config Release
if %errorlevel% neq 0 (
    echo Build failed!
    cd ..
    pause
    exit /b 1
)

echo.
echo ========================================
echo         Build completed successfully!
echo ========================================
echo Executable location: build\Release\AccountSvr.exe
echo.

cd ..
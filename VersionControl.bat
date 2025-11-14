@echo off
REM Setup script for Motion Engine Version Control System (Windows)

echo Setting up Version Control System for Motion Engine...

REM Check if we're in the project root
if not exist "CMakeLists.txt" (
    echo Error: Please run this script from your project root directory
    exit /b 1
)

REM Create cmake directory if it doesn't exist
if not exist "cmake" (
    echo Creating cmake directory...
    mkdir cmake
)

REM Get script directory
set SCRIPT_DIR=%~dp0

echo Copying GenerateVersionHeader.cmake...
copy "%SCRIPT_DIR%cmake\GenerateVersionHeader.cmake" cmake\ >nul

echo Copying Version.h.in...
copy "%SCRIPT_DIR%config\MotionVersion.hpp.in" cmake\ >nul

echo Backing up original CMakeLists.txt...
copy CMakeLists.txt CMakeLists.txt.backup >nul

echo Updating CMakeLists.txt...
copy "%SCRIPT_DIR%CMakeLists.txt" .\ >nul

echo.
echo ✓ Version control system installed successfully!
echo.
echo Next steps:
echo 1. Review the changes to CMakeLists.txt
echo 2. Reconfigure your build: cmake -B build
echo 3. Build your project: cmake --build build
echo 4. Include 'Version.h' in your code to access version info
echo.
echo See VERSION_CONTROL_README.md for detailed usage instructions.
echo.
echo Your original CMakeLists.txt has been backed up to CMakeLists.txt.backup

pause
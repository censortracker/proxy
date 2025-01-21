@ECHO OFF

CHCP 1252

REM %VAR:"=% mean dequoted %VAR%

set PATH=%QT_BIN_DIR:"=%;%PATH%

echo "Using Qt in %QT_BIN_DIR%"

REM Hold on to current directory
set PROJECT_DIR=%cd%
set SCRIPT_DIR=%PROJECT_DIR:"=%\deploy

set WORK_DIR=%SCRIPT_DIR:"=%\build_%BUILD_ARCH:"=%
set APP_NAME=DesktopProxy
set APP_FILENAME=%APP_NAME:"=%.exe
set OUT_APP_DIR=%WORK_DIR:"=%\release
set PREBILT_DIR=%PROJECT_DIR:"=%\xray-prebuilt\windows
set TARGET_FILENAME=%PROJECT_DIR:"=%\%APP_NAME:"=%_x%BUILD_ARCH:"=%.exe

echo "Environment:"
echo "WORK_DIR:             %WORK_DIR%"
echo "APP_FILENAME:         %APP_FILENAME%"
echo "PROJECT_DIR:          %PROJECT_DIR%"
echo "SCRIPT_DIR:           %SCRIPT_DIR%"
echo "OUT_APP_DIR:          %OUT_APP_DIR%"
echo "PREBILT_DIR:          %PREBILT_DIR%"
echo "TARGET_FILENAME:      %TARGET_FILENAME%"

echo "Cleanup..."
rmdir /Q /S %WORK_DIR%
del %TARGET_FILENAME%

mkdir %WORK_DIR%

call "%QT_BIN_DIR:"=%\qt-cmake" --version
cmake --version

cd %PROJECT_DIR%
echo "Configuring CMake project..."
call cmake . -B %WORK_DIR% "-DCMAKE_BUILD_TYPE:STRING=Release" "-DCMAKE_PREFIX_PATH:PATH=%QT_BIN_DIR%"

echo "Starting build..."
cd %WORK_DIR%
cmake --build . --config release --verbose -- /p:UseMultiToolTask=true /m
if %errorlevel% neq 0 exit /b %errorlevel%

echo "Deploying..."

mkdir "%OUT_APP_DIR%"
copy "%WORK_DIR%\proxyserver\Release\%APP_FILENAME%" "%OUT_APP_DIR%"

echo "Copying prebuilt data..."
xcopy %PREBILT_DIR%\*    %OUT_APP_DIR%  /s /e /y /i /f

echo "Deploy finished, content:"
dir %OUT_APP_DIR%

echo "Finished, see %OUT_APP_DIR%"
exit 0 
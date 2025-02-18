@ECHO OFF

CHCP 1252

REM %VAR:"=% mean dequoted %VAR%

set PATH=%QT_BIN_DIR:"=%;%PATH%

echo "Using Qt in %QT_BIN_DIR%"
echo "Using QIF in %QIF_BIN_DIR%"

REM Hold on to current directory
set PROJECT_DIR=%cd%
set SCRIPT_DIR=%PROJECT_DIR:"=%\deploy

set WORK_DIR=%SCRIPT_DIR:"=%\build_%BUILD_ARCH:"=%
set APP_NAME=CensorTrackerProxy
set APP_FILENAME=%APP_NAME:"=%.exe
set APP_DOMAIN=org.censortracker.proxy
set OUT_APP_DIR=%WORK_DIR:"=%\release
set PREBILT_DIR=%PROJECT_DIR:"=%\xray-prebuilt\windows
set INSTALLER_DATA_DIR=%WORK_DIR:"=%\installer\packages\%APP_DOMAIN:"=%\data
set TARGET_FILENAME=%PROJECT_DIR:"=%\%APP_NAME:"=%_x%BUILD_ARCH:"=%.exe

echo "Environment:"
echo "WORK_DIR:             %WORK_DIR%"
echo "APP_FILENAME:         %APP_FILENAME%"
echo "PROJECT_DIR:          %PROJECT_DIR%"
echo "SCRIPT_DIR:           %SCRIPT_DIR%"
echo "OUT_APP_DIR:          %OUT_APP_DIR%"
echo "PREBILT_DIR:          %PREBILT_DIR%"
echo "INSTALLER_DATA_DIR:   %INSTALLER_DATA_DIR%"
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
copy "%WORK_DIR%\CensorTrackerProxy\Release\*.*" "%OUT_APP_DIR%"

echo "Deploying Qt dependencies..."
"%QT_BIN_DIR:"=%\windeployqt" --release --force --no-translations "%OUT_APP_DIR:"=%\%APP_FILENAME:"=%"

echo "Deploy finished, content:"
dir %OUT_APP_DIR%

echo "Creating installer..."
cd %SCRIPT_DIR%
xcopy %SCRIPT_DIR:"=%\installer  %WORK_DIR:"=%\installer /s /e /y /i /f
mkdir %INSTALLER_DATA_DIR%

cd %OUT_APP_DIR%
echo "Compressing data..."
"%QIF_BIN_DIR:"=%\archivegen" -c 9 %INSTALLER_DATA_DIR:"=%\%APP_NAME:"=%.7z .

cd "%WORK_DIR:"=%\installer"
echo "Building installer..."
"%QIF_BIN_DIR:"=%\binarycreator" --offline-only -v -c config\windows.xml -p packages -f %TARGET_FILENAME%

timeout 5

echo "Finished, see %TARGET_FILENAME%"
exit 0
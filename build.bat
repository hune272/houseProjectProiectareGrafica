@echo off
REM Build script for Lab2 project using Visual Studio Developer Command Prompt

echo Setting up Visual Studio environment...
call "C:\Program Files\Microsoft Visual Studio\2022\Community\Common7\Tools\VsDevCmd.bat"

echo.
echo Navigating to build directory...
cd /d "D:\clion\PG\lab2\cmake-build-debug"

echo.
echo Building project with Ninja...
"C:\Program Files\JetBrains\CLion 2024.2.2\bin\ninja\win\x64\ninja.exe" lab2

echo.
if %ERRORLEVEL% EQU 0 (
    echo Build successful!
    echo Executable: D:\clion\PG\lab2\cmake-build-debug\lab2.exe
) else (
    echo Build failed with error code: %ERRORLEVEL%
)

pause

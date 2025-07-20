@echo off
REM Get the current folder name as the game name
for %%i in ("%cd%") do set "GAME_NAME=%%~ni"
echo Building %GAME_NAME% Game (Release) in C++23...

REM Define tool paths
set "CMAKE_PATH=C:\Program Files\CMake\bin\cmake.exe"
set "MINGW_PATH=C:\msys64\ucrt64\bin"

REM Check if cmake is available
where cmake >nul 2>&1
if %ERRORLEVEL% neq 0 (
    if exist "%CMAKE_PATH%" (
        echo Using CMake from: %CMAKE_PATH%
        set "CMAKE_COMMAND=%CMAKE_PATH%"
    ) else (
        echo Error: CMake is not installed or not in PATH
        exit /b 1
    )
) else (
    set "CMAKE_COMMAND=cmake"
)

REM Check if g++ is available
where g++ >nul 2>&1
if %ERRORLEVEL% neq 0 (
    if exist "%MINGW_PATH%\g++.exe" (
        echo Adding MinGW to PATH for this session: %MINGW_PATH%
        set "PATH=%MINGW_PATH%;%PATH%"
    ) else (
        echo Error: g++ is not installed or not in PATH
        pause
        exit /b 1
    )
)

REM Create build directory if it doesn't exist
if not exist "build\release" mkdir "build\release"

REM Configure and build
echo Configuring CMake...
"%CMAKE_COMMAND%" -B build/release -S . -DCMAKE_BUILD_TYPE=Release -G "MinGW Makefiles"
if %ERRORLEVEL% neq 0 (
    echo CMake configuration failed!
    exit /b 1
)

echo Building project...
"%CMAKE_COMMAND%" --build build/release --config Release
if %ERRORLEVEL% neq 0 (
    echo Build failed!
    exit /b 1
)

echo.
echo Build completed successfully!
echo Executable location: build\release\bin\%GAME_NAME%.exe
pause

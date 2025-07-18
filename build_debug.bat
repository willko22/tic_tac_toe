@echo off
REM Get the current folder name as the game name
for %%i in ("%cd%") do set "GAME_NAME=%%~ni"
echo Building %GAME_NAME% Game (Debug) with C++23, EnTT, and Sokol...

REM Define tool paths (update these if your installations are different)
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
        echo Please install CMake or add it to your PATH
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
        echo Please install MinGW-w64 or add it to your PATH
        exit /b 1
    )
)

REM Create build directory if it doesn't exist
if not exist "build\debug" mkdir "build\debug"


REM Configure and build
echo Configuring CMake...
set "CC=C:/msys64/ucrt64/bin/gcc.exe"
set "CXX=C:/msys64/ucrt64/bin/g++.exe"
set "CMAKE_MAKE_PROGRAM=C:/msys64/ucrt64/bin/mingw32-make.exe"
"%CMAKE_COMMAND%" -B build/debug -S . -DCMAKE_BUILD_TYPE=Debug -G "MinGW Makefiles" -DCMAKE_C_COMPILER="%CC%" -DCMAKE_CXX_COMPILER="%CXX%" -DCMAKE_MAKE_PROGRAM="%CMAKE_MAKE_PROGRAM%"
if %ERRORLEVEL% neq 0 (
    echo CMake configuration failed!
    exit /b 1
)

echo Building project...
"%CMAKE_COMMAND%" --build build/debug --config Debug
if %ERRORLEVEL% neq 0 (
    echo Build failed!
    exit /b 1
)

echo.
echo Build completed successfully!
echo Executable location: build\debug\bin\%GAME_NAME%_debug.exe

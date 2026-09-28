@echo off
REM CppMT5 Runner Script
REM
REM Usage: run.bat [APIKey]

chcp 65001 >nul

set "ROOT=%~dp0"
if "%ROOT:~-1%"=="\" set "ROOT=%ROOT:~0,-1%"

set "CMAKE_BIN=cmake"
where cmake >nul 2>nul
if errorlevel 1 (
    if exist "C:\Program Files\Microsoft Visual Studio\2022\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe" (
        set "CMAKE_BIN=C:\Program Files\Microsoft Visual Studio\2022\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe"
    ) else (
        echo Error: cmake not found. Please install CMake or Visual Studio with C++ tools.
        exit /b 1
    )
)

echo Configuring CppMT5 build...
"%CMAKE_BIN%" -B "%ROOT%\build" -S "%ROOT%" -DBUILD_EXAMPLES=ON
if errorlevel 1 (
    echo CMake configuration failed!
    exit /b 1
)

echo Building CppMT5...
"%CMAKE_BIN%" --build "%ROOT%\build" --config Release
if errorlevel 1 (
    echo Build failed!
    exit /b 1
)

set "EXE_PATH=%ROOT%\build\Release\quickstart.exe"
if not exist "%EXE_PATH%" set "EXE_PATH=%ROOT%\build\quickstart.exe"

if not exist "%EXE_PATH%" (
    echo Error: quickstart executable not found in build directory.
    exit /b 1
)

echo Running CppMT5 Quickstart...
"%EXE_PATH%" %*

@echo off
echo ==========================================
echo Unreal MCP Project Rebuild Script
echo ==========================================

echo.
echo [1/4] Killing Unreal Editor processes...
taskkill /F /IM "UnrealEditor.exe" >nul 2>&1
taskkill /F /IM "UnrealEditor-Cmd.exe" >nul 2>&1
taskkill /F /IM "UnrealEditor-Win64-Debug.exe" >nul 2>&1
taskkill /F /IM "UnrealEditor-Win64-Development.exe" >nul 2>&1
echo Waiting for processes to terminate...
timeout /t 2 /nobreak >nul

echo.
echo [2/4] Cleaning intermediate files...
cd /d "e:\code\unreal-mcp\MCPGameProject"
if exist "Intermediate" (
    rmdir /S /Q "Intermediate" 2>nul
    echo Cleaned Intermediate folder
)
if exist "Binaries" (
    rmdir /S /Q "Binaries" 2>nul
    echo Cleaned Binaries folder
)
if exist "Plugins\UnrealMCP\Intermediate" (
    rmdir /S /Q "Plugins\UnrealMCP\Intermediate" 2>nul
    echo Cleaned Plugin Intermediate folder
)
if exist "Plugins\UnrealMCP\Binaries" (
    rmdir /S /Q "Plugins\UnrealMCP\Binaries" 2>nul
    echo Cleaned Plugin Binaries folder
)

echo.
echo [3/4] Building project...
"C:\Program Files\Epic Games\UE_5.7\Engine\Build\BatchFiles\Build.bat" MCPGameProjectEditor Win64 Development -Project="e:\code\unreal-mcp\MCPGameProject\MCPGameProject.uproject" -TargetType=Editor

echo.
echo [4/4] Build complete!
echo ==========================================
if %ERRORLEVEL% EQU 0 (
    echo Status: SUCCESS
) else (
    echo Status: FAILED
    echo Error code: %ERRORLEVEL%
)
echo ==========================================
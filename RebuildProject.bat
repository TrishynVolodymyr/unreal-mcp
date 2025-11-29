@echo off
echo Killing Unreal Editor processes...
taskkill /F /IM "UnrealEditor.exe" >nul 2>&1
taskkill /F /IM "UnrealEditor-Cmd.exe" >nul 2>&1
taskkill /F /IM "UnrealEditor-Win64-Debug.exe" >nul 2>&1
taskkill /F /IM "UnrealEditor-Win64-Development.exe" >nul 2>&1
echo Waiting for processes to terminate...
timeout /t 2 /nobreak >nul
echo Building project...
cd /d "e:\code\unreal-mcp\MCPGameProject"
"C:\Program Files\Epic Games\UE_5.7\Engine\Build\BatchFiles\Build.bat" MCPGameProjectEditor Win64 Development -Project="e:\code\unreal-mcp\MCPGameProject\MCPGameProject.uproject" -TargetType=Editor
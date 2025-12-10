@echo off
cd /d "e:\code\unreal-mcp\MCPGameProject"
"C:\Program Files\Epic Games\UE_5.7\Engine\Build\BatchFiles\Build.bat" MCPGameProjectEditor Win64 Development -Project="e:\code\unreal-mcp\MCPGameProject\MCPGameProject.uproject" -TargetType=Editor 2>&1 | powershell -Command "$input | Tee-Object -FilePath 'e:\code\unreal-mcp\build_output_latest.txt'"

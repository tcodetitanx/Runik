@echo off
rem Package Runik for Windows (Shipping). Usage:
rem   package_win64.bat ["C:\Program Files\Epic Games\UE_5.8"]
setlocal
set ENGINE=%~1
if "%ENGINE%"=="" set ENGINE=C:\Program Files\Epic Games\UE_5.8
set PROJECT=%~dp0..\Runik.uproject

call "%ENGINE%\Engine\Build\BatchFiles\RunUAT.bat" BuildCookRun ^
  -project="%PROJECT%" -platform=Win64 -clientconfig=Shipping ^
  -build -cook -stage -pak -archive ^
  -archivedirectory="%~dp0..\Packaged"

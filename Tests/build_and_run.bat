@echo off
rem Build and run the Runik core tests with the VS 2022 toolchain.
setlocal
where cl >nul 2>nul
if not errorlevel 1 goto :build
for /f "usebackq tokens=*" %%i in (`"%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe" -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath`) do set "VSDIR=%%i"
if not defined VSDIR (echo Visual Studio with C++ tools not found & exit /b 1)
call "%VSDIR%\VC\Auxiliary\Build\vcvars64.bat" >nul
:build
if not exist "%~dp0bin" mkdir "%~dp0bin"
cl /nologo /std:c++17 /EHsc /W3 /O2 ^
   "%~dp0CoreTests.cpp" ^
   "%~dp0..\Source\Runik\Core\RunikSim.cpp" ^
   "%~dp0..\Source\Runik\Core\RunikPuzzles.cpp" ^
   "%~dp0..\Source\Runik\Core\RunikGifEnc.cpp" ^
   /Fe:"%~dp0bin\CoreTests.exe" /Fo:"%~dp0bin\\"
if errorlevel 1 exit /b 1
"%~dp0bin\CoreTests.exe"

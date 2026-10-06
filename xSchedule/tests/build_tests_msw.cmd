@echo off
rem Builds and runs the schedule unit tests with Visual Studio (x64).
rem Expects the same layout as the main build: wxWidgets built in ..\..\..\wxWidgets.
setlocal
cd /d "%~dp0"

rem the "(x86)" in the path would end the for block early, so expand it outside the block
set "VSWHERE=%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe"
for /f "usebackq tokens=*" %%i in (`call "%VSWHERE%" -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath`) do set VSINSTALL=%%i
if "%VSINSTALL%"=="" (
    echo Visual Studio with C++ tools not found
    exit /b 2
)
call "%VSINSTALL%\VC\Auxiliary\Build\vcvars64.bat" >nul 2>&1

if not exist build mkdir build
cl /nologo /std:c++20 /EHsc /MD /O2 /utf-8 /W3 /DNDEBUG /DUNICODE /D_UNICODE /D__MSVC__ /DNOMINMAX /D_CRT_SECURE_NO_WARNINGS ^
   /I..\..\..\wxWidgets\include /I..\..\..\wxWidgets\include\msvc ^
   /I..\..\xlights\include /I..\..\xlights\xLights /I..\..\xlights\xLights\utils /I..\..\xlights\xLights\ui /I..\..\xlights\common ^
   /I..\..\dependencies /I..\..\dependencies\spdlog\include ^
   /Fobuild\ /Fe:build\xScheduleTests.exe ^
   ScheduleTests.cpp ..\Schedule.cpp ..\City.cpp ..\Holidays.cpp ^
   /link /LIBPATH:..\..\..\wxWidgets\lib\vc_x64_lib /SUBSYSTEM:CONSOLE
if errorlevel 1 exit /b 1

build\xScheduleTests.exe

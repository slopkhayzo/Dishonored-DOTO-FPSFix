@echo off
setlocal EnableExtensions EnableDelayedExpansion
cd /d "%~dp0"

where cl.exe >nul 2>nul
if errorlevel 1 (
  set "VSWHERE=%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe"
  if not exist "!VSWHERE!" (
    echo Microsoft C++ Build Tools were not found.
    exit /b 1
  )

  set "VSINSTALL="
  for /f "usebackq tokens=*" %%I in (`"!VSWHERE!" -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath`) do set "VSINSTALL=%%I"
  if not defined VSINSTALL (
    echo The Visual Studio x64 C++ toolchain was not found.
    exit /b 1
  )
  call "!VSINSTALL!\Common7\Tools\VsDevCmd.bat" -arch=x64 -host_arch=x64
  if errorlevel 1 exit /b 1
)

if not exist "build" mkdir "build"

cl.exe /nologo /std:c++17 /O2 /EHsc /W4 ^
  /Fo:"build\joint-pose-test.obj" ^
  "joint-pose-test.cpp" ^
  /link /OUT:"build\joint-pose-test.exe"
if errorlevel 1 exit /b 1

"build\joint-pose-test.exe"
if errorlevel 1 exit /b 1

cl.exe /nologo /std:c++17 /O2 /EHsc /W4 /LD /DUNICODE /D_UNICODE ^
  /Fo:"build\\" ^
  "dinput8_proxy.cpp" ^
  /link /DEF:"dinput8_proxy.def" /OUT:"build\dinput8.dll" ^
  /IMPLIB:"build\dinput8_proxy.lib" bcrypt.lib
if errorlevel 1 exit /b 1

cl.exe /nologo /std:c++17 /O2 /EHsc /W4 /DUNICODE /D_UNICODE ^
  /Fo:"build\proxy-smoke-test.obj" ^
  "proxy-smoke-test.cpp" ^
  /link /OUT:"build\proxy-smoke-test.exe" dinput8.lib dxguid.lib
if errorlevel 1 exit /b 1

echo.
echo Built DOTO high-FPS proxy:
echo   %CD%\build\dinput8.dll
echo   %CD%\build\joint-pose-test.exe
echo   %CD%\build\proxy-smoke-test.exe
endlocal

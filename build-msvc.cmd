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

cl.exe /nologo /std:c++17 /O2 /EHsc /W4 /DUNICODE /D_UNICODE ^
  /Fo:"build\adaptive-controller-test.obj" ^
  "adaptive-controller-test.cpp" ^
  /link /OUT:"build\adaptive-controller-test.exe" bcrypt.lib
if errorlevel 1 exit /b 1

"build\adaptive-controller-test.exe"
if errorlevel 1 exit /b 1

cl.exe /nologo /std:c++17 /O2 /EHsc /W4 /LD /DUNICODE /D_UNICODE ^
  /Fo:"build\\" ^
  "doto_high_fps_fix.cpp" ^
  /link /OUT:"build\DOTOHighFPSFix.asi" bcrypt.lib
if errorlevel 1 exit /b 1

cl.exe /nologo /std:c++17 /O2 /EHsc /W4 /DUNICODE /D_UNICODE ^
  /Fo:"build\asi-load-test.obj" ^
  "asi-load-test.cpp" ^
  /link /OUT:"build\asi-load-test.exe"
if errorlevel 1 exit /b 1

set "ASI_SPACE_TEST_DIR=build\ASI path with spaces"
if not exist "!ASI_SPACE_TEST_DIR!" mkdir "!ASI_SPACE_TEST_DIR!"
copy /y "build\DOTOHighFPSFix.asi" "!ASI_SPACE_TEST_DIR!\DOTOHighFPSFix.asi" >nul
if errorlevel 1 exit /b 1

"build\asi-load-test.exe" "!ASI_SPACE_TEST_DIR!\DOTOHighFPSFix.asi"
if errorlevel 1 exit /b 1

echo.
echo Built DOTO high-FPS ASI plugin:
echo   %CD%\build\DOTOHighFPSFix.asi
echo   %CD%\build\joint-pose-test.exe
echo   %CD%\build\adaptive-controller-test.exe
echo   %CD%\build\asi-load-test.exe
endlocal

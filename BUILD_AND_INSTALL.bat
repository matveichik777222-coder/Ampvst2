@echo off
setlocal
cd /d "%~dp0"
title AmpForge - build and install

:: self-elevate (needed to copy into Program Files)
net session >nul 2>&1
if %errorlevel% neq 0 (
  powershell -NoProfile -Command "Start-Process -FilePath '%~f0' -Verb RunAs"
  exit /b
)

set NEED_RESTART=0
where git >nul 2>&1 || (echo Installing Git... & winget install -e --id Git.Git --silent --accept-package-agreements --accept-source-agreements & set NEED_RESTART=1)
where cmake >nul 2>&1 || (echo Installing CMake... & winget install -e --id Kitware.CMake --silent --accept-package-agreements --accept-source-agreements & set NEED_RESTART=1)
if not exist "%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe" (
  echo Installing Visual Studio Build Tools, this takes a while...
  winget install -e --id Microsoft.VisualStudio.2022.BuildTools --silent --accept-package-agreements --accept-source-agreements --override "--wait --quiet --add Microsoft.VisualStudio.Workload.VCTools --includeRecommended"
  set NEED_RESTART=1
)
if "%NEED_RESTART%"=="1" (
  echo.
  echo Tools installed. CLOSE this window and run BUILD_AND_INSTALL.bat once more.
  pause
  exit /b
)

echo Building (first build downloads JUCE, ~10-15 min)...
cmake -B build -G "Visual Studio 17 2022" -A x64 || goto :fail
cmake --build build --config Release --target AmpForge_VST3 AmpForge_Standalone --parallel || goto :fail

set "SRC=build\AmpForge_artefacts\Release\VST3\AmpForge.vst3"
set "DST=%CommonProgramFiles%\VST3"
if not exist "%DST%" mkdir "%DST%"
xcopy /E /I /Y "%SRC%" "%DST%\AmpForge.vst3" >nul || goto :fail

echo.
echo DONE. Installed to %DST%\AmpForge.vst3
echo Standalone: build\AmpForge_artefacts\Release\Standalone\AmpForge.exe
echo Now rescan plugins in FL Studio / Reaper.
pause
exit /b

:fail
echo.
echo BUILD FAILED. Copy the red error text above and send it to Claude.
pause
exit /b 1

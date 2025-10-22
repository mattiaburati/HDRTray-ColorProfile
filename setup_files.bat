@echo off
REM Setup script for HDRTray with color profile management
REM This script should be run after building HDRTray to copy the necessary external tools and profiles

echo ========================================
echo HDRTray Color Profile Setup
echo ========================================
echo.

REM Get the installation directory (where HDRTray.exe is located)
set "INSTALL_DIR=%~dp0build\Release"

REM Check if the installation directory exists
if not exist "%INSTALL_DIR%" (
    echo ERROR: Installation directory not found: %INSTALL_DIR%
    echo Please build the project first using CMake
    pause
    exit /b 1
)

echo Installation directory: %INSTALL_DIR%
echo.

REM Create necessary directories
echo Creating directories...
if not exist "%INSTALL_DIR%\bin" mkdir "%INSTALL_DIR%\bin"
if not exist "%INSTALL_DIR%\profiles" mkdir "%INSTALL_DIR%\profiles"
echo - bin directory created
echo - profiles directory created
echo.

REM Instructions for copying files
echo ========================================
echo MANUAL SETUP REQUIRED
echo ========================================
echo.
echo Please copy the following files manually:
echo.
echo 1. Copy to "%INSTALL_DIR%\bin\":
echo    - dispwin.exe (from ArgyllCMS)
echo    - winddcutil.exe
echo.
echo 2. Copy to "%INSTALL_DIR%\profiles\":
echo    - Xiaomi 27i Pro_Rtings.icm (SDR ICC profile)
echo    - xiaomi_miniled_1d.cal (HDR calibration file)
echo.
echo ========================================
echo.

REM Check if files are present
echo Checking for required files...
set "ALL_FILES_PRESENT=1"

if not exist "%INSTALL_DIR%\bin\dispwin.exe" (
    echo [MISSING] dispwin.exe
    set "ALL_FILES_PRESENT=0"
) else (
    echo [OK] dispwin.exe found
)

if not exist "%INSTALL_DIR%\bin\winddcutil.exe" (
    echo [MISSING] winddcutil.exe
    set "ALL_FILES_PRESENT=0"
) else (
    echo [OK] winddcutil.exe found
)

if not exist "%INSTALL_DIR%\profiles\Xiaomi 27i Pro_Rtings.icm" (
    echo [MISSING] Xiaomi 27i Pro_Rtings.icm
    set "ALL_FILES_PRESENT=0"
) else (
    echo [OK] Xiaomi 27i Pro_Rtings.icm found
)

if not exist "%INSTALL_DIR%\profiles\xiaomi_miniled_1d.cal" (
    echo [MISSING] xiaomi_miniled_1d.cal
    set "ALL_FILES_PRESENT=0"
) else (
    echo [OK] xiaomi_miniled_1d.cal found
)

echo.

if "%ALL_FILES_PRESENT%"=="1" (
    echo ========================================
    echo Setup complete! All files are present.
    echo ========================================
    echo.
    echo You can now run HDRTray.exe
    echo Color profile management will be enabled automatically.
) else (
    echo ========================================
    echo Setup incomplete!
    echo ========================================
    echo.
    echo Please copy the missing files listed above.
    echo HDRTray will work without these files, but color profile
    echo management features will be disabled.
)

echo.
echo Where to find the required tools:
echo.
echo dispwin.exe:
echo   - Download ArgyllCMS from https://www.argyllcms.com/
echo   - Extract and find dispwin.exe in the bin folder
echo.
echo winddcutil.exe:
echo   - Download from https://github.com/rockowitz/ddcutil
echo   - Or search for Windows DDC/CI control utilities
echo.
pause

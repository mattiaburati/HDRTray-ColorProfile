@echo off
REM Example script to copy profiles from the Xiaomi 27i project to HDRTray
REM Modify the paths according to your directory structure

echo Copying ICC profiles and calibration files to HDRTray...
echo.

REM Set the source directory (Xiaomi 27i project)
set "SOURCE_DIR=..\Xiaomi 27i_Automated HDR"

REM Set the destination directory (HDRTray build output)
set "DEST_DIR=build\Release"

REM Check if source directory exists
if not exist "%SOURCE_DIR%" (
    echo ERROR: Source directory not found: %SOURCE_DIR%
    echo Please adjust the SOURCE_DIR variable in this script
    pause
    exit /b 1
)

REM Check if destination directory exists
if not exist "%DEST_DIR%" (
    echo ERROR: Destination directory not found: %DEST_DIR%
    echo Please build HDRTray first
    pause
    exit /b 1
)

REM Create profiles directory if it doesn't exist
if not exist "%DEST_DIR%\profiles" mkdir "%DEST_DIR%\profiles"

REM Copy ICC profiles
if exist "%SOURCE_DIR%\Xiaomi 27i Pro_Rtings.icm" (
    copy "%SOURCE_DIR%\Xiaomi 27i Pro_Rtings.icm" "%DEST_DIR%\profiles\"
    echo [OK] Copied Xiaomi 27i Pro_Rtings.icm
) else (
    echo [MISSING] Xiaomi 27i Pro_Rtings.icm not found in source directory
)

if exist "%SOURCE_DIR%\Xiaomi 27i Pro_DisplayCal.icm" (
    copy "%SOURCE_DIR%\Xiaomi 27i Pro_DisplayCal.icm" "%DEST_DIR%\profiles\"
    echo [OK] Copied Xiaomi 27i Pro_DisplayCal.icm
) else (
    echo [MISSING] Xiaomi 27i Pro_DisplayCal.icm not found in source directory
)

REM Copy calibration file (if it exists)
if exist "%SOURCE_DIR%\xiaomi_miniled_1d.cal" (
    copy "%SOURCE_DIR%\xiaomi_miniled_1d.cal" "%DEST_DIR%\profiles\"
    echo [OK] Copied xiaomi_miniled_1d.cal
) else (
    echo [WARNING] xiaomi_miniled_1d.cal not found in source directory
    echo You may need to create this file using DisplayCAL or ArgyllCMS
)

echo.
echo Done! Profiles copied to %DEST_DIR%\profiles\
echo.
echo Next steps:
echo 1. Copy dispwin.exe and winddcutil.exe to %DEST_DIR%\bin\
echo 2. Run setup_files.bat to verify the installation
echo.
pause

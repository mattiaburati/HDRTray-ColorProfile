# Color Profile Management Setup

HDRTray now includes automatic color profile management and monitor calibration when switching between HDR and SDR modes.

## Features

- **Automatic ICC Profile Loading**: Loads the correct color profile when switching to SDR mode
- **HDR Calibration**: Applies calibration settings when enabling HDR mode
- **DDC/CI Monitor Control**: Automatically adjusts monitor settings (brightness, color gains) via DDC/CI

## Directory Structure

After building and installing HDRTray, you need the following directory structure:

```
HDRTray/
├── HDRTray.exe
├── HDRCmd.exe
├── bin/
│   ├── dispwin.exe      (from ArgyllCMS)
│   └── winddcutil.exe   (DDC/CI control utility)
└── profiles/
    ├── Xiaomi 27i Pro_Rtings.icm     (SDR ICC profile)
    └── xiaomi_miniled_1d.cal          (HDR calibration file)
```

## Required External Tools

### 1. dispwin.exe (ArgyllCMS)
**Purpose**: Loads ICC color profiles and calibration files

**Download**:
- Website: https://www.argyllcms.com/
- Download the Windows version
- Extract the archive and locate `dispwin.exe` in the `bin` folder
- Copy `dispwin.exe` to `HDRTray\bin\dispwin.exe`

### 2. winddcutil.exe
**Purpose**: Controls monitor settings via DDC/CI protocol

**Download**:
- GitHub: https://github.com/rockowitz/ddcutil
- Look for Windows builds or alternatives like:
  - ControlMyMonitor by NirSoft
  - ddcset
  - WinDDCUtil
- Copy the executable to `HDRTray\bin\winddcutil.exe`

**Note**: If using an alternative tool, you may need to modify `ColorProfileManager.cpp` to use the correct command syntax.

## Profile Files

### SDR Profile: Xiaomi 27i Pro_Rtings.icm
This is the ICC color profile used in SDR mode. You can use:
- A profile calibrated with DisplayCAL or similar tools
- A manufacturer-provided profile
- A profile from review sites (like Rtings)

Place this file in: `HDRTray\profiles\Xiaomi 27i Pro_Rtings.icm`

### HDR Calibration: xiaomi_miniled_1d.cal
This is the calibration file for HDR mode, typically a 1D LUT generated with ArgyllCMS/DisplayCAL.

Place this file in: `HDRTray\profiles\xiaomi_miniled_1d.cal`

## Setup Instructions

### Option 1: Automated Setup (Recommended)

1. Build the HDRTray project with CMake
2. Run `setup_files.bat` from the project root
3. Follow the on-screen instructions to copy the required files
4. The script will verify that all files are in place

### Option 2: Manual Setup

1. Build the HDRTray project
2. Navigate to the build output directory (typically `build\Release`)
3. Create two folders: `bin` and `profiles`
4. Copy the external tools to the `bin` folder
5. Copy your profile files to the `profiles` folder
6. Run HDRTray.exe

## Monitor Settings

The following DDC/CI VCP codes are used to control the monitor:

### SDR Mode Settings
- `0x10` (Brightness): 50
- `0x16` (Video Gain Red): 50
- `0x18` (Video Gain Green): 49
- `0x1A` (Video Gain Blue): 49

### HDR Mode Settings
- `0x14` (Select Color Preset): 12
- `0x10` (Brightness): 100
- `0x16` (Video Gain Red): 46
- `0x18` (Video Gain Green): 49
- `0x1A` (Video Gain Blue): 49

## Customization

To customize the settings for your monitor:

1. Open `ColorProfileManager.hpp` and modify the constants:
   - Change `SDR_PROFILE` to your SDR profile filename
   - Change `HDR_CALIBRATION` to your HDR calibration filename
   - Change `DISPLAY_ID` if needed (usually 1 for primary monitor)

2. Open `ColorProfileManager.cpp` and modify the VCP values in:
   - `ApplySDRProfile()` - for SDR mode settings
   - `ApplyHDRCalibration()` - for HDR mode settings

3. Rebuild the project

## Troubleshooting

### Color profiles are not loading

1. Check the debug output with a tool like DebugView
2. Verify that `dispwin.exe` and `winddcutil.exe` are in the `bin` folder
3. Verify that profile files are in the `profiles` folder
4. Ensure the profile filenames match exactly (case-sensitive)

### Monitor settings are not changing

1. Verify your monitor supports DDC/CI
2. Enable DDC/CI in your monitor's OSD menu
3. Test `winddcutil.exe` manually from command line
4. Check if your monitor uses different VCP codes (refer to your monitor's DDC/CI documentation)

### HDRTray still works without the tools

Yes! If the external tools or profiles are not found, HDRTray will still function normally for toggling HDR on/off. Color profile management will simply be disabled with a warning in the debug output.

## For Developers

The color profile management is implemented in:
- `ColorProfileManager.hpp` - Interface and configuration
- `ColorProfileManager.cpp` - Implementation
- `NotifyIcon.cpp` - Integration with HDR toggle logic

The integration is designed to be non-intrusive:
- If tools are not available, HDRTray continues to work normally
- All operations are logged to debug output for troubleshooting
- The implementation follows the same coding style as the rest of HDRTray

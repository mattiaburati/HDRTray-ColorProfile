HDRTray
=======
Windows Notification Area icon to show and change HDR status.
Useful if you quickly want to check the current setting, maybe because it might been changed automatically by some applications, or maybe just because you're forgetful.
Also allows quick toggling.

Usage
-----
After starting the program you can see a new notification area icon, displaying
either “SDR” or “HDR” depending on whether HDR is off or on.

HDR can be toggled on or off with a left-click on the icon.

Right-clicking opens the context menu offering an option to automatically start
the program when you log in to Windows.

Color Profile Management (New!)
--------------------------------
HDRTray now includes automatic color profile and monitor calibration management when switching between HDR and SDR modes.

### Features
- **Automatic ICC Profile Loading**: Applies different ICC profiles for SDR and HDR modes
- **DDC/CI Monitor Control**: Automatically adjusts monitor settings (brightness, RGB gains, color presets)
- **Configurable Settings**: All settings can be customized via the `HDRTray.ini` configuration file
- **Status Indicator**: Menu shows "[OK] Color Tools: Ready" when color management tools are properly configured

### Setup
1. Place `dispwin.exe` (from ArgyllCMS) and `winddcutil.exe` in the `bin/` folder next to HDRTray.exe
2. Place your ICC profiles (.icm files) and calibration files (.cal files) in the `profiles/` folder
3. Right-click the HDRTray icon and select "Settings..." to configure your preferences

### Configuration
The `HDRTray.ini` file allows you to configure all color management settings. You can access it by right-clicking the tray icon and selecting "Settings..." or by editing it directly.

**Note**: Configuration changes are automatically reloaded on every HDR/SDR toggle, so there's no need to restart the application after modifying the INI file.

```ini
[Monitor]
DisplayId=1

[Profiles]
; Master toggle for ALL color management features (1=enabled, 0=disabled)
EnableColorManagement=1
SDRProfile=YourSDRProfile.icm
HDRCalibration=YourHDRCalibration.cal
; Enable/disable profile loading (1=enabled, 0=disabled)
EnableSDRProfile=1
EnableHDRProfile=1
; Enable/disable color preset change with HDR toggle (requires extra OFF/ON cycle)
EnableColorPresetChange=0

[SDR]
Brightness=50
RedGain=50
GreenGain=49
BlueGain=49

[HDR]
Brightness=100
ColorPreset=12
RedGain=46
GreenGain=49
BlueGain=49
```

#### Profile Toggle Options
You can enable/disable each feature via the right-click menu:
- **Enable Color Management**: Master toggle to enable/disable ALL color management features at once
- **Apply SDR Profile**: Load ICC profile when switching to SDR mode
- **Apply HDR Profile**: Load calibration file when switching to HDR mode
- **Change Color Preset**: Enable monitor color preset change (requires extra HDR toggle cycle)

These settings are also configurable via checkboxes in the system tray menu for quick access.

**Note**: If "Enable Color Management" is unchecked, all color management features will be disabled and HDRTray will only toggle HDR on/off without applying any profiles or monitor settings.

#### VCP Codes Reference
All VCP codes are standard DDC/CI values:
- `0x10` - Brightness
- `0x14` - Color Preset Selection
- `0x16` - Video Gain: Red
- `0x18` - Video Gain: Green
- `0x1A` - Video Gain: Blue

Command line utility
--------------------
Since version 0.5, the `HDRCmd` command line utility is included. It can be used to toggle HDR on and off from scripts and check it's status.

Syntax:

    HDRCmd [SUBCOMMAND] [SUBCOMMAND-OPTIONS]

## `on` command
Turns HDR on on all supported displays.

Does not accept any options.

## `off` command
Turns HDR off on all supported displays.

Does not accept any options.

## `status` command
Prints the current HDR status to the console. Has a special mode that returns an exit code depending on the status.

### `--mode` (`-m`) option
Specifies how the status should be reported. Accepts the following values:

* `short`, `s` (default): Print a single line indicating the overall HDR status.
* `long`, `l`: Print the overall HDR status and status per display.
* `exitcode`, `x`: Special mode for scripting. Exit code is 0 if HDR is on, 1 if HDR is off, and 2 if HDR is unsupported. (Other values indicate some error.)

Contributed scripts
-------------------
A number of people shared scripts they created that use `HDRCmd` to automate HDR toggling. Check them out in the [“Show and Tell” discussion category](https://github.com/res2k/HDRTray/discussions/categories/show-and-tell).

If you want to share your own script, feel free to open a new discussion thread!

Latest Release
--------------
The latest release can be found on the [“Releases” tab of the GitHub project page](https://github.com/res2k/HDRTray/releases).

Feedback, Issues
----------------
Please use the [“Issues” tab of the GitHub project page](https://github.com/res2k/HDRTray/issues) to provide feedback or report any issues you may encounter.

License
-------
HDRTray, a notification icon for the "Use HDR" option

Original Copyright (C) 2022-2025 Frank Richter

Color Profile Management fork Copyright (C) 2025 Mattia Burati

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

[Full text of GNU General Public License, Version 3](LICENSE.md).

Credits
-------
- **Original HDRTray**: [Frank Richter](https://github.com/res2k/HDRTray)
- **Color Profile Management Extension**: Mattia Burati (2025)
  - Automatic ICC profile switching
  - DDC/CI monitor control integration
  - Configurable color management settings
  - Profile toggle controls

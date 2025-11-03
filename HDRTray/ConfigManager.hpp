/*
    HDRTray, a notification icon for the "Use HDR" option
    Copyright (C) 2022 Frank Richter

    Configuration Management
    Copyright (C) 2025 Mattia Burati

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/

#pragma once

#include <string>
#include <windows.h>

/**
 * Configuration manager for HDRTray color profile settings.
 * Uses an INI file to store user preferences.
 */
class ConfigManager
{
public:
    struct MonitorSettings
    {
        int displayId = 1;

        // Profile filenames
        std::wstring sdrProfileName = L"Xiaomi 27i Pro_Rtings.icm";
        std::wstring hdrCalibrationName = L"xiaomi_miniled_1d.cal";

        // Master toggle for all color management features
        bool enableColorManagement = true;

        // Profile enable/disable toggles
        bool enableSdrProfile = true;
        bool enableHdrProfile = true;
        bool enableColorPresetChange = false;  // Enable changing monitor color preset for HDR

        // SDR settings
        int sdrBrightness = 50;
        int sdrRedGain = 50;
        int sdrGreenGain = 49;
        int sdrBlueGain = 49;

        // HDR settings
        int hdrBrightness = 100;
        int hdrRedGain = 46;
        int hdrGreenGain = 49;
        int hdrBlueGain = 49;
        int hdrColorPreset = 12;
    };

    ConfigManager();
    ~ConfigManager();

    /**
     * Load configuration from INI file
     * @return true if successful
     */
    bool Load();

    /**
     * Save configuration to INI file
     * @return true if successful
     */
    bool Save();

    /**
     * Get current monitor settings
     */
    const MonitorSettings& GetMonitorSettings() const { return m_monitorSettings; }

    /**
     * Set monitor settings
     */
    void SetMonitorSettings(const MonitorSettings& settings);

    /**
     * Get config file path
     */
    std::wstring GetConfigFilePath() const { return m_configFilePath; }

private:
    std::wstring m_configFilePath;
    MonitorSettings m_monitorSettings;

    std::wstring GetExecutablePath() const;
    int ReadIntValue(const wchar_t* section, const wchar_t* key, int defaultValue);
    bool ReadBoolValue(const wchar_t* section, const wchar_t* key, bool defaultValue);
    std::wstring ReadStringValue(const wchar_t* section, const wchar_t* key, const wchar_t* defaultValue);
    bool WriteIntValue(const wchar_t* section, const wchar_t* key, int value);
    bool WriteBoolValue(const wchar_t* section, const wchar_t* key, bool value);
    bool WriteStringValue(const wchar_t* section, const wchar_t* key, const wchar_t* value);
};

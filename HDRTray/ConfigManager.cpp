/*
    HDRTray, a notification icon for the "Use HDR" option
    Copyright (C) 2022 Frank Richter

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

#include "ConfigManager.hpp"
#include <shlwapi.h>

#pragma comment(lib, "shlwapi.lib")

ConfigManager::ConfigManager()
{
    // Get config file path (in the same directory as the executable)
    std::wstring exePath = GetExecutablePath();
    m_configFilePath = exePath + L"\\HDRTray.ini";
}

ConfigManager::~ConfigManager()
{
}

std::wstring ConfigManager::GetExecutablePath() const
{
    wchar_t path[MAX_PATH];
    GetModuleFileNameW(nullptr, path, MAX_PATH);
    PathRemoveFileSpecW(path);
    return std::wstring(path);
}

bool ConfigManager::Load()
{
    // Check if config file exists
    if (!PathFileExistsW(m_configFilePath.c_str()))
    {
        // Create default config
        return Save();
    }

    // Load monitor settings
    m_monitorSettings.displayId = ReadIntValue(L"Monitor", L"DisplayId", 1);

    // Load profile filenames
    m_monitorSettings.sdrProfileName = ReadStringValue(L"Profiles", L"SDRProfile", L"Xiaomi 27i Pro_Rtings.icm");
    m_monitorSettings.hdrCalibrationName = ReadStringValue(L"Profiles", L"HDRCalibration", L"xiaomi_miniled_1d.cal");

    m_monitorSettings.sdrBrightness = ReadIntValue(L"SDR", L"Brightness", 50);
    m_monitorSettings.sdrRedGain = ReadIntValue(L"SDR", L"RedGain", 50);
    m_monitorSettings.sdrGreenGain = ReadIntValue(L"SDR", L"GreenGain", 49);
    m_monitorSettings.sdrBlueGain = ReadIntValue(L"SDR", L"BlueGain", 49);

    m_monitorSettings.hdrBrightness = ReadIntValue(L"HDR", L"Brightness", 100);
    m_monitorSettings.hdrRedGain = ReadIntValue(L"HDR", L"RedGain", 46);
    m_monitorSettings.hdrGreenGain = ReadIntValue(L"HDR", L"GreenGain", 49);
    m_monitorSettings.hdrBlueGain = ReadIntValue(L"HDR", L"BlueGain", 49);
    m_monitorSettings.hdrColorPreset = ReadIntValue(L"HDR", L"ColorPreset", 12);

    return true;
}

bool ConfigManager::Save()
{
    // Save monitor settings
    if (!WriteIntValue(L"Monitor", L"DisplayId", m_monitorSettings.displayId))
        return false;

    // Save profile filenames
    if (!WriteStringValue(L"Profiles", L"SDRProfile", m_monitorSettings.sdrProfileName.c_str()))
        return false;
    if (!WriteStringValue(L"Profiles", L"HDRCalibration", m_monitorSettings.hdrCalibrationName.c_str()))
        return false;

    if (!WriteIntValue(L"SDR", L"Brightness", m_monitorSettings.sdrBrightness))
        return false;
    if (!WriteIntValue(L"SDR", L"RedGain", m_monitorSettings.sdrRedGain))
        return false;
    if (!WriteIntValue(L"SDR", L"GreenGain", m_monitorSettings.sdrGreenGain))
        return false;
    if (!WriteIntValue(L"SDR", L"BlueGain", m_monitorSettings.sdrBlueGain))
        return false;

    if (!WriteIntValue(L"HDR", L"Brightness", m_monitorSettings.hdrBrightness))
        return false;
    if (!WriteIntValue(L"HDR", L"RedGain", m_monitorSettings.hdrRedGain))
        return false;
    if (!WriteIntValue(L"HDR", L"GreenGain", m_monitorSettings.hdrGreenGain))
        return false;
    if (!WriteIntValue(L"HDR", L"BlueGain", m_monitorSettings.hdrBlueGain))
        return false;
    if (!WriteIntValue(L"HDR", L"ColorPreset", m_monitorSettings.hdrColorPreset))
        return false;

    return true;
}

void ConfigManager::SetMonitorSettings(const MonitorSettings& settings)
{
    m_monitorSettings = settings;
}

int ConfigManager::ReadIntValue(const wchar_t* section, const wchar_t* key, int defaultValue)
{
    return GetPrivateProfileIntW(section, key, defaultValue, m_configFilePath.c_str());
}

bool ConfigManager::WriteIntValue(const wchar_t* section, const wchar_t* key, int value)
{
    wchar_t buffer[32];
    swprintf_s(buffer, L"%d", value);
    return WritePrivateProfileStringW(section, key, buffer, m_configFilePath.c_str()) != 0;
}

std::wstring ConfigManager::ReadStringValue(const wchar_t* section, const wchar_t* key, const wchar_t* defaultValue)
{
    wchar_t buffer[MAX_PATH];
    GetPrivateProfileStringW(section, key, defaultValue, buffer, MAX_PATH, m_configFilePath.c_str());
    return std::wstring(buffer);
}

bool ConfigManager::WriteStringValue(const wchar_t* section, const wchar_t* key, const wchar_t* value)
{
    return WritePrivateProfileStringW(section, key, value, m_configFilePath.c_str()) != 0;
}

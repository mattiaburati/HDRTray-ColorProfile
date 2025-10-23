/*
    HDRTray, a notification icon for the "Use HDR" option
    Copyright (C) 2022 Frank Richter

    Color Profile Management Extension
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

#include "ColorProfileManager.hpp"
#include "ConfigManager.hpp"
#include "Resource.h"
#include <windows.h>
#include <shlwapi.h>
#include <filesystem>

#pragma comment(lib, "shlwapi.lib")

extern HINSTANCE hInst; // From HDRTray.cpp

ColorProfileManager::ColorProfileManager()
    : m_toolsExtracted(false)
    , m_useEmbeddedTools(false)
    , m_config(nullptr)
{
    // Load configuration
    m_config = new ConfigManager();
    m_config->Load();
    // Get the executable directory
    m_executablePath = GetExecutablePath();

    // Set up paths relative to executable
    m_binPath = m_executablePath + L"\\bin";
    m_profilesPath = m_executablePath + L"\\profiles";

    // Get temp directory for extracted resources
    wchar_t tempPath[MAX_PATH];
    GetTempPathW(MAX_PATH, tempPath);
    m_tempPath = std::wstring(tempPath) + L"HDRTray_Tools";

    // Set tool paths
    m_dispwinPath = GetToolPath(L"dispwin.exe");
    m_winddcutilPath = GetToolPath(L"winddcutil.exe");

    // Check if external tools exist
    bool externalToolsExist = PathFileExistsW(m_dispwinPath.c_str()) &&
                              PathFileExistsW(m_winddcutilPath.c_str());

    if (!externalToolsExist) {
        // Try to extract embedded resources
        OutputDebugStringW(L"External tools not found, attempting to extract embedded resources...\n");
        if (ExtractEmbeddedTools()) {
            m_useEmbeddedTools = true;
            m_dispwinPath = m_tempPath + L"\\dispwin.exe";
            m_winddcutilPath = m_tempPath + L"\\winddcutil.exe";
            OutputDebugStringW(L"Embedded tools extracted successfully\n");
        } else {
            OutputDebugStringW(L"Failed to extract embedded tools\n");
        }
    } else {
        OutputDebugStringW(L"Using external tools from bin folder\n");
    }
}

ColorProfileManager::~ColorProfileManager()
{
    // Clean up temporary files if we extracted embedded resources
    if (m_useEmbeddedTools && m_toolsExtracted) {
        CleanupTemporaryFiles();
    }

    // Clean up config
    if (m_config) {
        delete m_config;
        m_config = nullptr;
    }
}

std::wstring ColorProfileManager::GetExecutablePath() const
{
    wchar_t path[MAX_PATH];
    GetModuleFileNameW(nullptr, path, MAX_PATH);
    PathRemoveFileSpecW(path);
    return std::wstring(path);
}

std::wstring ColorProfileManager::GetToolPath(const wchar_t* toolName) const
{
    return m_binPath + L"\\" + toolName;
}

std::wstring ColorProfileManager::GetProfilePath(const wchar_t* profileName) const
{
    return m_profilesPath + L"\\" + profileName;
}

bool ColorProfileManager::AreToolsAvailable() const
{
    return PathFileExistsW(m_dispwinPath.c_str()) &&
           PathFileExistsW(m_winddcutilPath.c_str());
}

bool ColorProfileManager::ExecuteCommand(const std::wstring& command) const
{
    STARTUPINFOW si = { sizeof(STARTUPINFOW) };
    PROCESS_INFORMATION pi = {};

    si.dwFlags = STARTF_USESHOWWINDOW;
    si.wShowWindow = SW_HIDE;

    // CreateProcess requires a modifiable string
    std::wstring cmdLine = command;

    BOOL result = CreateProcessW(
        nullptr,
        &cmdLine[0],
        nullptr,
        nullptr,
        FALSE,
        CREATE_NO_WINDOW,
        nullptr,
        nullptr,
        &si,
        &pi
    );

    if (!result)
    {
        OutputDebugStringW((L"Failed to execute command: " + command + L"\n").c_str());
        return false;
    }

    // Wait for process to complete
    WaitForSingleObject(pi.hProcess, INFINITE);

    DWORD exitCode = 0;
    GetExitCodeProcess(pi.hProcess, &exitCode);

    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);

    return exitCode == 0;
}

bool ColorProfileManager::LoadICCProfile(const wchar_t* profilePath) const
{
    const auto& settings = m_config->GetMonitorSettings();
    std::wstring command = L"\"" + m_dispwinPath + L"\" -d " +
                          std::to_wstring(settings.displayId) + L" \"" +
                          profilePath + L"\"";

    OutputDebugStringW((L"Loading ICC profile: " + command + L"\n").c_str());
    return ExecuteCommand(command);
}

bool ColorProfileManager::SetMonitorVCP(int display, int vcpCode, int value) const
{
    // Format VCP code as hexadecimal (e.g., 0x10, not 0x16)
    wchar_t vcpHex[8];
    swprintf_s(vcpHex, L"%X", vcpCode);

    std::wstring command = L"\"" + m_winddcutilPath + L"\" setvcp " +
                          std::to_wstring(display) + L" 0x" +
                          vcpHex + L" " +
                          std::to_wstring(value);

    OutputDebugStringW((L"Setting VCP: " + command + L"\n").c_str());
    return ExecuteCommand(command);
}

void ColorProfileManager::Sleep(int milliseconds) const
{
    ::Sleep(milliseconds);
}

bool ColorProfileManager::ApplySDRProfile()
{
    if (!AreToolsAvailable())
    {
        OutputDebugStringW(L"Color profile tools not available\n");
        return false;
    }

    OutputDebugStringW(L"Applying SDR profile and settings\n");

    // Get settings from config
    const auto& settings = m_config->GetMonitorSettings();

    // Load SDR ICC profile (optional - skip if disabled or file doesn't exist)
    if (settings.enableSdrProfile)
    {
        if (!settings.sdrProfileName.empty())
        {
            std::wstring sdrProfilePath = GetProfilePath(settings.sdrProfileName.c_str());
            if (PathFileExistsW(sdrProfilePath.c_str()))
            {
                if (!LoadICCProfile(sdrProfilePath.c_str()))
                {
                    OutputDebugStringW(L"Warning: Failed to load SDR ICC profile\n");
                    // Continue anyway - not a critical error
                }
            }
            else
            {
                OutputDebugStringW((L"SDR profile not found: " + sdrProfilePath + L" (skipping)\n").c_str());
            }
        }
        else
        {
            OutputDebugStringW(L"No SDR profile configured (skipping)\n");
        }
    }
    else
    {
        OutputDebugStringW(L"SDR profile disabled (skipping)\n");
    }

    // Apply SDR monitor settings via DDC/CI (from config)
    SetMonitorVCP(settings.displayId, 0x10, settings.sdrBrightness);  // Brightness
    SetMonitorVCP(settings.displayId, 0x16, settings.sdrRedGain);     // Video Gain Red
    SetMonitorVCP(settings.displayId, 0x18, settings.sdrGreenGain);   // Video Gain Green
    SetMonitorVCP(settings.displayId, 0x1A, settings.sdrBlueGain);    // Video Gain Blue

    OutputDebugStringW(L"SDR settings applied successfully\n");
    return true;
}

bool ColorProfileManager::PrepareForHDR()
{
    if (!AreToolsAvailable())
    {
        OutputDebugStringW(L"Color profile tools not available\n");
        return false;
    }

    OutputDebugStringW(L"Preparing monitor for HDR mode\n");

    // Get settings from config
    const auto& settings = m_config->GetMonitorSettings();

    // Wait before starting calibration (same as batch file: timeout 3)
    Sleep(3000);

    // Set monitor to specific color preset for HDR
    OutputDebugStringW(L"Setting HDR color preset\n");
    SetMonitorVCP(settings.displayId, 0x14, settings.hdrColorPreset);

    return true;
}

bool ColorProfileManager::ApplyHDRCalibration()
{
    if (!AreToolsAvailable())
    {
        OutputDebugStringW(L"Color profile tools not available\n");
        return false;
    }

    OutputDebugStringW(L"Applying HDR calibration and settings\n");

    // Get settings from config
    const auto& settings = m_config->GetMonitorSettings();

    // NOTE: The caller (NotifyIcon::ToggleHDR) should have already:
    // 1. Enabled HDR
    // 2. Called PrepareForHDR() (which waits 1s and sets color preset 0x14)
    // 3. Toggled HDR OFF then ON again
    // This function continues from that point

    // Load HDR calibration file (optional - skip if disabled or file doesn't exist)
    if (settings.enableHdrProfile)
    {
        if (!settings.hdrCalibrationName.empty())
        {
            std::wstring hdrCalibrationPath = GetProfilePath(settings.hdrCalibrationName.c_str());
            if (PathFileExistsW(hdrCalibrationPath.c_str()))
            {
                std::wstring command = L"\"" + m_dispwinPath + L"\" -d " +
                                      std::to_wstring(settings.displayId) + L" \"" +
                                      hdrCalibrationPath + L"\"";

                OutputDebugStringW((L"Loading HDR calibration: " + command + L"\n").c_str());
                if (!ExecuteCommand(command))
                {
                    OutputDebugStringW(L"Warning: Failed to load HDR calibration\n");
                    // Continue anyway
                }
            }
            else
            {
                OutputDebugStringW((L"HDR calibration not found: " + hdrCalibrationPath + L" (skipping)\n").c_str());
            }
        }
        else
        {
            OutputDebugStringW(L"No HDR calibration configured (skipping)\n");
        }
    }
    else
    {
        OutputDebugStringW(L"HDR profile disabled (skipping)\n");
    }

    Sleep(1000);  // Same as batch file: timeout 1

    // Apply HDR monitor settings via DDC/CI (from config)
    OutputDebugStringW(L"Setting HDR brightness\n");
    SetMonitorVCP(settings.displayId, 0x10, settings.hdrBrightness);  // Brightness

    Sleep(2000);  // Same as batch file: timeout 2

    OutputDebugStringW(L"Setting HDR RGB gains\n");
    SetMonitorVCP(settings.displayId, 0x16, settings.hdrRedGain);     // Video Gain Red
    SetMonitorVCP(settings.displayId, 0x18, settings.hdrGreenGain);   // Video Gain Green
    SetMonitorVCP(settings.displayId, 0x1A, settings.hdrBlueGain);    // Video Gain Blue

    OutputDebugStringW(L"HDR calibration applied successfully\n");
    return true;
}

bool ColorProfileManager::ExtractEmbeddedResource(int resourceId, const wchar_t* resourceType, const std::wstring& outputPath)
{
    // Find the resource
    HRSRC hResource = FindResourceW(hInst, MAKEINTRESOURCEW(resourceId), resourceType);
    if (!hResource) {
        OutputDebugStringW((L"Failed to find resource " + std::to_wstring(resourceId) + L"\n").c_str());
        return false;
    }

    // Load the resource
    HGLOBAL hLoadedResource = LoadResource(hInst, hResource);
    if (!hLoadedResource) {
        OutputDebugStringW(L"Failed to load resource\n");
        return false;
    }

    // Lock the resource to get a pointer to the data
    LPVOID pResourceData = LockResource(hLoadedResource);
    if (!pResourceData) {
        OutputDebugStringW(L"Failed to lock resource\n");
        return false;
    }

    // Get the size of the resource
    DWORD resourceSize = SizeofResource(hInst, hResource);
    if (resourceSize == 0) {
        OutputDebugStringW(L"Resource has zero size\n");
        return false;
    }

    // Create output file
    HANDLE hFile = CreateFileW(outputPath.c_str(), GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (hFile == INVALID_HANDLE_VALUE) {
        OutputDebugStringW((L"Failed to create output file: " + outputPath + L"\n").c_str());
        return false;
    }

    // Write resource data to file
    DWORD bytesWritten;
    BOOL writeResult = WriteFile(hFile, pResourceData, resourceSize, &bytesWritten, nullptr);
    CloseHandle(hFile);

    if (!writeResult || bytesWritten != resourceSize) {
        OutputDebugStringW(L"Failed to write resource to file\n");
        DeleteFileW(outputPath.c_str());
        return false;
    }

    OutputDebugStringW((L"Successfully extracted resource to: " + outputPath + L"\n").c_str());
    return true;
}

bool ColorProfileManager::ExtractEmbeddedTools()
{
    // Create temporary directory if it doesn't exist
    CreateDirectoryW(m_tempPath.c_str(), nullptr);

    // Extract dispwin.exe
    std::wstring dispwinTempPath = m_tempPath + L"\\dispwin.exe";
    if (!ExtractEmbeddedResource(IDR_DISPWIN_EXE, L"BINARY", dispwinTempPath)) {
        OutputDebugStringW(L"Failed to extract dispwin.exe (resource may not be embedded)\n");
        return false;
    }

    // Extract winddcutil.exe
    std::wstring winddcutilTempPath = m_tempPath + L"\\winddcutil.exe";
    if (!ExtractEmbeddedResource(IDR_WINDDCUTIL_EXE, L"BINARY", winddcutilTempPath)) {
        OutputDebugStringW(L"Failed to extract winddcutil.exe (resource may not be embedded)\n");
        DeleteFileW(dispwinTempPath.c_str());
        return false;
    }

    m_toolsExtracted = true;
    return true;
}

void ColorProfileManager::CleanupTemporaryFiles()
{
    if (m_tempPath.empty())
        return;

    OutputDebugStringW(L"Cleaning up temporary tool files...\n");

    // Delete extracted executables
    DeleteFileW((m_tempPath + L"\\dispwin.exe").c_str());
    DeleteFileW((m_tempPath + L"\\winddcutil.exe").c_str());

    // Try to remove the directory (will only succeed if empty)
    RemoveDirectoryW(m_tempPath.c_str());
}

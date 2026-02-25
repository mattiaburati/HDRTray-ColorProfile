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
#include <algorithm>
#include <vector>
#include <regex>

#pragma comment(lib, "shlwapi.lib")

extern HINSTANCE hInst; // From HDRTray.cpp

static bool TryMultiByteToWide(UINT codePage, DWORD flags, const std::string& input, std::wstring& output)
{
    if (input.empty())
    {
        output.clear();
        return true;
    }

    int size = MultiByteToWideChar(codePage, flags, input.c_str(), -1, nullptr, 0);
    if (size <= 0)
        return false;

    std::vector<wchar_t> buffer(size);
    size = MultiByteToWideChar(codePage, flags, input.c_str(), -1, buffer.data(), size);
    if (size <= 0)
        return false;

    output.assign(buffer.data());
    return true;
}

static std::wstring MultiByteToWideBestEffort(const std::string& input)
{
    std::wstring output;

    // Try UTF-8 first (some tools emit UTF-8), then fall back to OEM/ANSI codepages
    if (TryMultiByteToWide(CP_UTF8, MB_ERR_INVALID_CHARS, input, output))
        return output;

    if (TryMultiByteToWide(GetOEMCP(), 0, input, output))
        return output;

    (void)TryMultiByteToWide(GetACP(), 0, input, output);
    return output;
}

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

bool ColorProfileManager::ExecuteCommandWithOutput(const std::wstring& command, std::wstring& output) const
{
    SECURITY_ATTRIBUTES sa = {};
    sa.nLength = sizeof(SECURITY_ATTRIBUTES);
    sa.bInheritHandle = TRUE;
    sa.lpSecurityDescriptor = nullptr;

    // Create pipes for stdout
    HANDLE hReadPipe = nullptr;
    HANDLE hWritePipe = nullptr;
    if (!CreatePipe(&hReadPipe, &hWritePipe, &sa, 0))
    {
        OutputDebugStringW(L"Failed to create pipe\n");
        return false;
    }

    // Ensure read handle is not inherited
    SetHandleInformation(hReadPipe, HANDLE_FLAG_INHERIT, 0);

    STARTUPINFOW si = { sizeof(STARTUPINFOW) };
    PROCESS_INFORMATION pi = {};

    si.dwFlags = STARTF_USESHOWWINDOW | STARTF_USESTDHANDLES;
    si.wShowWindow = SW_HIDE;
    si.hStdOutput = hWritePipe;
    si.hStdError = hWritePipe;

    // CreateProcess requires a modifiable string
    std::wstring cmdLine = command;

    BOOL result = CreateProcessW(
        nullptr,
        &cmdLine[0],
        nullptr,
        nullptr,
        TRUE, // Inherit handles
        CREATE_NO_WINDOW,
        nullptr,
        nullptr,
        &si,
        &pi
    );

    if (!result)
    {
        OutputDebugStringW((L"Failed to execute command: " + command + L"\n").c_str());
        CloseHandle(hReadPipe);
        CloseHandle(hWritePipe);
        return false;
    }

    // Close write pipe in parent process
    CloseHandle(hWritePipe);

    // Read output from pipe
    char buffer[4096];
    DWORD bytesRead;
    std::string outputStr;

    while (ReadFile(hReadPipe, buffer, sizeof(buffer) - 1, &bytesRead, nullptr) && bytesRead > 0)
    {
        buffer[bytesRead] = '\0';
        outputStr += buffer;
    }

    // Convert to wstring
    if (!outputStr.empty())
    {
        output = MultiByteToWideBestEffort(outputStr);
    }

    // Wait for process to complete
    WaitForSingleObject(pi.hProcess, INFINITE);

    DWORD exitCode = 0;
    GetExitCodeProcess(pi.hProcess, &exitCode);

    CloseHandle(hReadPipe);
    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);

    return exitCode == 0;
}

bool ColorProfileManager::LoadICCProfile(const wchar_t* profilePath) const
{
    const auto& settings = m_config->GetMonitorSettings();

    // Check file extension to determine if we need -I flag
    // .cal files don't need -I flag, .icc/.icm files do
    std::wstring path(profilePath);
    std::wstring extension;
    size_t dotPos = path.find_last_of(L'.');
    if (dotPos != std::wstring::npos) {
        extension = path.substr(dotPos);
        // Convert to lowercase for comparison
        std::transform(extension.begin(), extension.end(), extension.begin(), ::towlower);
    }

    std::wstring command = L"\"" + m_dispwinPath + L"\"";

    // Only add -I flag for .icc or .icm files (ICC profile installation)
    // .cal files are calibration files and should be loaded without -I flag
    if (extension == L".icc" || extension == L".icm") {
        command += L" -I";
    }

    command += L" -d " + std::to_wstring(settings.displayId) + L" \"" + profilePath + L"\"";

    OutputDebugStringW((L"Loading color profile: " + command + L"\n").c_str());
    return ExecuteCommand(command);
}

static bool TryParseVcpCurrentValue(const std::wstring& output, int& currentValue)
{
    // Try to capture formats like:
    // - "current value = 50"
    // - "current=50"
    // - "current value: 0x0032"
    // This avoids relying on a specific tool (winddcutil vs renamed alternatives).
    static const std::wregex reCurrent(
        LR"((?:^|\s)current(?:\s+value)?\s*[:=]?\s*(0x[0-9a-fA-F]+|\d+))",
        std::regex_constants::icase);

    std::wsmatch match;
    if (std::regex_search(output, match, reCurrent) && match.size() >= 2)
    {
        const std::wstring token = match[1].str();
        try
        {
            if (token.rfind(L"0x", 0) == 0 || token.rfind(L"0X", 0) == 0)
                currentValue = std::stoi(token, nullptr, 16);
            else
                currentValue = std::stoi(token, nullptr, 10);
            return true;
        }
        catch (...)
        {
            return false;
        }
    }

    // Handle terse outputs like:
    // - "VCP 0x10 50"
    // - "VCP 0x10 0x32"
    // Some tools print: "VCP <code> <current> [max]" without labels.
    static const std::wregex reTerseVcp(
        LR"((?:^|\s)VCP\s+0x[0-9a-fA-F]+\s+(0x[0-9a-fA-F]+|\d+)(?:\s|$))",
        std::regex_constants::icase);

    if (std::regex_search(output, match, reTerseVcp) && match.size() >= 2)
    {
        const std::wstring token = match[1].str();
        try
        {
            if (token.rfind(L"0x", 0) == 0 || token.rfind(L"0X", 0) == 0)
                currentValue = std::stoi(token, nullptr, 16);
            else
                currentValue = std::stoi(token, nullptr, 10);
            return true;
        }
        catch (...)
        {
            return false;
        }
    }

    // Fallback: some tools print "VCP ...: value = X, max = Y" (without "current")
    static const std::wregex reValue(
        LR"((?:^|\s)(?:value|val)\s*[:=]\s*(0x[0-9a-fA-F]+|\d+))",
        std::regex_constants::icase);

    if (std::regex_search(output, match, reValue) && match.size() >= 2)
    {
        const std::wstring token = match[1].str();
        try
        {
            if (token.rfind(L"0x", 0) == 0 || token.rfind(L"0X", 0) == 0)
                currentValue = std::stoi(token, nullptr, 16);
            else
                currentValue = std::stoi(token, nullptr, 10);
            return true;
        }
        catch (...)
        {
            return false;
        }
    }

    return false;
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

bool ColorProfileManager::GetMonitorVCP(int display, int vcpCode, int& currentValue) const
{
    // Format VCP code as hexadecimal
    wchar_t vcpHex[8];
    swprintf_s(vcpHex, L"%X", vcpCode);

    std::wstring command = L"\"" + m_winddcutilPath + L"\" getvcp " +
                          std::to_wstring(display) + L" 0x" +
                          vcpHex;

    OutputDebugStringW((L"Getting VCP: " + command + L"\n").c_str());

    std::wstring output;
    if (!ExecuteCommandWithOutput(command, output))
    {
        OutputDebugStringW(L"Failed to execute getvcp command\n");
        return false;
    }

    if (!TryParseVcpCurrentValue(output, currentValue))
    {
        OutputDebugStringW((L"Could not parse getvcp output: " + output + L"\n").c_str());
        return false;
    }

    OutputDebugStringW((L"Current VCP value: " + std::to_wstring(currentValue) + L"\n").c_str());
    return true;
}

bool ColorProfileManager::SetMonitorVCPVerified(int display, int vcpCode, int value, int maxRetries) const
{
    constexpr int kSetVerifySettleDelayMs = 200;
    const int kRetryBackoffMs[] = { 150, 300, 500 };
    const int kRetryBackoffCount = static_cast<int>(sizeof(kRetryBackoffMs) / sizeof(kRetryBackoffMs[0]));

    for (int attempt = 0; attempt < maxRetries; attempt++)
    {
        if (attempt > 0)
        {
            OutputDebugStringW((L"Retry attempt " + std::to_wstring(attempt + 1) + L" of " + std::to_wstring(maxRetries) + L"\n").c_str());
        }

        // Set the VCP value
        if (!SetMonitorVCP(display, vcpCode, value))
        {
            OutputDebugStringW(L"Failed to set VCP value\n");
            if (attempt < maxRetries - 1)
            {
                const int backoffIndex = std::min(attempt, kRetryBackoffCount - 1);
                Sleep(kRetryBackoffMs[backoffIndex]);
                continue;
            }
            return false;
        }

        // Wait briefly for the monitor to settle before verification
        Sleep(kSetVerifySettleDelayMs);

        // Verify the value was set correctly
        int currentValue = -1;
        if (GetMonitorVCP(display, vcpCode, currentValue))
        {
            if (currentValue == value)
            {
                OutputDebugStringW(L"VCP value verified successfully\n");
                return true;
            }
            else
            {
                OutputDebugStringW((L"VCP value mismatch: expected " + std::to_wstring(value) +
                                   L", got " + std::to_wstring(currentValue) + L"\n").c_str());
                if (attempt < maxRetries - 1)
                {
                    const int backoffIndex = std::min(attempt, kRetryBackoffCount - 1);
                    Sleep(kRetryBackoffMs[backoffIndex]);
                }
            }
        }
        else
        {
            OutputDebugStringW(L"Failed to verify VCP value (getvcp failed)\n");
            // Continue anyway as getvcp might not be supported for all codes
            if (attempt == maxRetries - 1)
            {
                OutputDebugStringW(L"Warning: Could not verify VCP value, assuming success\n");
                return true;
            }

            const int backoffIndex = std::min(attempt, kRetryBackoffCount - 1);
            Sleep(kRetryBackoffMs[backoffIndex]);
        }
    }

    OutputDebugStringW(L"Failed to set and verify VCP value after all retries\n");
    return false;
}

bool ColorProfileManager::EnsureVcp14ColorMode(int display) const
{
    // VCP 0x14 is known to be the color mode selector on supported displays.
    // Read it only after DDC/CI is ready to avoid false negatives during transitions.
    if (!WaitForVcpReadable(display, 0x14, /*timeoutMs=*/10000, /*pollMs=*/250))
    {
        OutputDebugStringW(L"DDC/CI not ready (getvcp probe timed out) for VCP 0x14\n");
        return false;
    }

    int currentValue = -1;

    if (!GetMonitorVCP(display, 0x14, currentValue))
    {
        OutputDebugStringW(L"Failed to read VCP 0x14 before color correction despite readiness probe\n");
        return false;
    }

    if (currentValue == 12)
    {
        OutputDebugStringW(L"VCP 0x14 already set to 12\n");
        return true;
    }

    OutputDebugStringW(L"Setting VCP 0x14 to 12 before applying color correction\n");
    if (!SetMonitorVCPVerified(display, 0x14, 12))
    {
        OutputDebugStringW(L"Failed to set VCP 0x14 to 12, aborting color correction path\n");
        return false;
    }

    // Re-check after the write settles; abort if the value is still not readable/12.
    if (!WaitForVcpReadable(display, 0x14, /*timeoutMs=*/10000, /*pollMs=*/250))
    {
        OutputDebugStringW(L"DDC/CI became unreadable after setting VCP 0x14\n");
        return false;
    }

    constexpr int kVcp14StabilizationWindowMs = 2500;
    constexpr int kVcp14StabilizationPollMs = 250;
    constexpr int kVcp14RequiredConsecutiveReads = 2;

    int consecutiveReads = 0;
    bool stabilized = false;
    DWORD stabilizationStartTick = GetTickCount();

    while (static_cast<int>(GetTickCount() - stabilizationStartTick) < kVcp14StabilizationWindowMs)
    {
        if (GetMonitorVCP(display, 0x14, currentValue))
        {
            if (currentValue == 12)
            {
                consecutiveReads++;
                if (consecutiveReads >= kVcp14RequiredConsecutiveReads)
                {
                    stabilized = true;
                    break;
                }
            }
            else
            {
                consecutiveReads = 0;
            }
        }
        else
        {
            OutputDebugStringW(L"Failed to re-read VCP 0x14 during stabilization\n");
            consecutiveReads = 0;
        }

        Sleep(kVcp14StabilizationPollMs);
    }

    if (!stabilized)
    {
        OutputDebugStringW((L"VCP 0x14 did not stabilize at 12 (last value " + std::to_wstring(currentValue) + L")\n").c_str());
        return false;
    }

    OutputDebugStringW(L"VCP 0x14 set to 12 successfully\n");
    return true;
}

bool ColorProfileManager::WaitForVcpReadable(int display, int vcpCode, int timeoutMs, int pollMs) const
{
    const DWORD startTick = GetTickCount();
    int currentValue = -1;

    while (static_cast<int>(GetTickCount() - startTick) < timeoutMs)
    {
        if (GetMonitorVCP(display, vcpCode, currentValue))
            return true;
        Sleep(pollMs);
    }
    return false;
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

    // Wait for monitor to switch to SDR mode before applying profile
    // Increased delay from 1s to 3s to ensure monitor is fully stabilized
    // This fixes the issue where brightness is not applied when switching from HDR->SDR
    OutputDebugStringW(L"Waiting 3 seconds for monitor to switch to SDR mode...\n");
    Sleep(3000);

    // Load SDR ICC profile (optional - skip if disabled or file doesn't exist)
    if (settings.enableSdrProfile)
    {
        if (!settings.sdrProfileName.empty())
        {
            std::wstring sdrProfilePath = GetProfilePath(settings.sdrProfileName.c_str());
            if (PathFileExistsW(sdrProfilePath.c_str()))
            {
                OutputDebugStringW(L"Loading SDR ICC profile...\n");
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

    if (!EnsureVcp14ColorMode(settings.displayId))
    {
        OutputDebugStringW(L"Aborting SDR color correction because VCP 0x14 could not be ensured\n");
        return false;
    }

    // Apply SDR monitor calibrations via DDC/CI (from config)
    OutputDebugStringW(L"Applying SDR calibrations (brightness and RGB gains)...\n");
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
    // 2. Called PrepareForHDR() (which waits 3s and sets color preset 0x14) if enableColorPresetChange
    // 3. Toggled HDR OFF then ON again if enableColorPresetChange
    // This function continues from that point

    // Wait for monitor to switch to HDR mode before applying profile
    // Increased delay from 1s to 3s to ensure monitor is fully stabilized
    // This fixes the issue where brightness is not applied when switching from SDR->HDR
    // after the system started in HDR mode
    OutputDebugStringW(L"Waiting 3 seconds for monitor to switch to HDR mode...\n");
    Sleep(3000);

    // Load HDR calibration file (optional - skip if disabled or file doesn't exist)
    if (settings.enableHdrProfile)
    {
        if (!settings.hdrCalibrationName.empty())
        {
            std::wstring hdrCalibrationPath = GetProfilePath(settings.hdrCalibrationName.c_str());
            if (PathFileExistsW(hdrCalibrationPath.c_str()))
            {
                OutputDebugStringW(L"Loading HDR calibration...\n");
                if (!LoadICCProfile(hdrCalibrationPath.c_str()))
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

    // Apply HDR monitor calibrations via DDC/CI (from config)
    OutputDebugStringW(L"Applying HDR calibrations (brightness and RGB gains)...\n");
    SetMonitorVCP(settings.displayId, 0x10, settings.hdrBrightness);  // Brightness
    SetMonitorVCP(settings.displayId, 0x16, settings.hdrRedGain);     // Video Gain Red
    SetMonitorVCP(settings.displayId, 0x18, settings.hdrGreenGain);   // Video Gain Green
    SetMonitorVCP(settings.displayId, 0x1A, settings.hdrBlueGain);    // Video Gain Blue

    OutputDebugStringW(L"HDR calibration applied successfully\n");
    return true;
}

bool ColorProfileManager::ReapplyHDRColorCorrection()
{
    return ReapplyHDRColorCorrection(/*force=*/true);
}

bool ColorProfileManager::ReapplySDRColorCorrection()
{
    return ReapplySDRColorCorrection(/*force=*/true);
}

bool ColorProfileManager::ReapplyHDRColorCorrection(bool force)
{
    if (!AreToolsAvailable())
    {
        OutputDebugStringW(L"Color profile tools not available\n");
        return false;
    }

    OutputDebugStringW(L"Reapplying HDR color correction (DDC/CI only)...\n");

    const auto& settings = m_config->GetMonitorSettings();

    // The monitor might be "on" but not yet ready to accept/read DDC/CI after signal restore.
    // Probe using a generally-supported VCP (brightness) and wait a bit.
    if (!WaitForVcpReadable(settings.displayId, 0x10, /*timeoutMs=*/15000, /*pollMs=*/500))
    {
        OutputDebugStringW(L"DDC/CI not ready (getvcp probe timed out), skipping HDR reapply\n");
        return false;
    }

    if (!force)
    {
        int readableCount = 0;
        bool mismatch = false;

        struct Pair { int code; int desired; };
        const Pair desired[] = {
            { 0x10, settings.hdrBrightness },
            { 0x16, settings.hdrRedGain },
            { 0x18, settings.hdrGreenGain },
            { 0x1A, settings.hdrBlueGain },
        };

        for (const auto& item : desired)
        {
            int currentValue = -1;
            if (!GetMonitorVCP(settings.displayId, item.code, currentValue))
                continue;
            readableCount++;
            if (currentValue != item.desired)
            {
                mismatch = true;
                break;
            }
        }

        // Skip only if we can read *all* relevant values and they match.
        // If the tool/monitor can't report some VCPs, assume we might still need reapply.
        if (readableCount == static_cast<int>(std::size(desired)) && !mismatch)
        {
            OutputDebugStringW(L"HDR VCP values already match desired settings, skipping reapply\n");
            return true;
        }
    }

    OutputDebugStringW(L"Applying HDR calibrations (brightness and RGB gains) with verification...\n");
    bool success = true;
    success &= SetMonitorVCPVerified(settings.displayId, 0x10, settings.hdrBrightness);  // Brightness
    success &= SetMonitorVCPVerified(settings.displayId, 0x16, settings.hdrRedGain);     // Video Gain Red
    success &= SetMonitorVCPVerified(settings.displayId, 0x18, settings.hdrGreenGain);   // Video Gain Green
    success &= SetMonitorVCPVerified(settings.displayId, 0x1A, settings.hdrBlueGain);    // Video Gain Blue

    if (success)
        OutputDebugStringW(L"HDR color correction reapplied successfully\n");
    else
        OutputDebugStringW(L"Warning: Some HDR color corrections may not have been applied correctly\n");

    return success;
}

bool ColorProfileManager::ReapplySDRColorCorrection(bool force)
{
    if (!AreToolsAvailable())
    {
        OutputDebugStringW(L"Color profile tools not available\n");
        return false;
    }

    OutputDebugStringW(L"Reapplying SDR color correction (DDC/CI only)...\n");

    const auto& settings = m_config->GetMonitorSettings();

    if (!WaitForVcpReadable(settings.displayId, 0x10, /*timeoutMs=*/15000, /*pollMs=*/500))
    {
        OutputDebugStringW(L"DDC/CI not ready (getvcp probe timed out), skipping SDR reapply\n");
        return false;
    }

    if (!force)
    {
        int readableCount = 0;
        bool mismatch = false;

        struct Pair { int code; int desired; };
        const Pair desired[] = {
            { 0x10, settings.sdrBrightness },
            { 0x16, settings.sdrRedGain },
            { 0x18, settings.sdrGreenGain },
            { 0x1A, settings.sdrBlueGain },
        };

        for (const auto& item : desired)
        {
            int currentValue = -1;
            if (!GetMonitorVCP(settings.displayId, item.code, currentValue))
                continue;
            readableCount++;
            if (currentValue != item.desired)
            {
                mismatch = true;
                break;
            }
        }

        if (readableCount == static_cast<int>(std::size(desired)) && !mismatch)
        {
            OutputDebugStringW(L"SDR VCP values already match desired settings, skipping reapply\n");
            return true;
        }
    }

    if (!EnsureVcp14ColorMode(settings.displayId))
    {
        OutputDebugStringW(L"Aborting SDR reapply because VCP 0x14 could not be ensured\n");
        return false;
    }

    OutputDebugStringW(L"Applying SDR calibrations (brightness and RGB gains) with verification...\n");
    bool success = true;
    success &= SetMonitorVCPVerified(settings.displayId, 0x10, settings.sdrBrightness);  // Brightness
    success &= SetMonitorVCPVerified(settings.displayId, 0x16, settings.sdrRedGain);     // Video Gain Red
    success &= SetMonitorVCPVerified(settings.displayId, 0x18, settings.sdrGreenGain);   // Video Gain Green
    success &= SetMonitorVCPVerified(settings.displayId, 0x1A, settings.sdrBlueGain);    // Video Gain Blue

    if (success)
        OutputDebugStringW(L"SDR color correction reapplied successfully\n");
    else
        OutputDebugStringW(L"Warning: Some SDR color corrections may not have been applied correctly\n");

    return success;
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

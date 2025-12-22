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

#pragma once

#include <string>
#include <optional>

// Forward declaration
class ConfigManager;

/**
 * Manager for color profile operations and monitor calibration.
 * Handles ICC profile loading and DDC/CI monitor control via external tools.
 */
class ColorProfileManager
{
public:
    ColorProfileManager();
    ~ColorProfileManager();

    /**
     * Apply SDR color profile and monitor settings
     * @return true if successful, false otherwise
     */
    bool ApplySDRProfile();

    /**
     * Apply HDR calibration and monitor settings
     * @return true if successful, false otherwise
     */
    bool ApplyHDRCalibration();

    /**
     * Check if the required tools are available
     * @return true if dispwin.exe and winddcutil.exe are found
     */
    bool AreToolsAvailable() const;

    /**
     * Prepare monitor for HDR mode by setting color preset
     * Should be called before toggling HDR on
     * @return true if successful, false otherwise
     */
    bool PrepareForHDR();

    /**
     * Reapply HDR color correction (DDC/CI only, no ICC/cal profiles)
     * Used when monitor reconnects after signal loss in HDR mode
     * @return true if successful, false otherwise
     */
    bool ReapplyHDRColorCorrection();

    /**
     * Reapply SDR color correction (DDC/CI only, no ICC/cal profiles)
     * Used when monitor reconnects after signal loss in SDR mode
     * @return true if successful, false otherwise
     */
    bool ReapplySDRColorCorrection();

    // Variants used for monitor reconnection handling:
    // - force=true: always reapply (useful after standby/resume where the monitor may glitch without changing VCP values)
    // - force=false: only reapply if a readable VCP value mismatches the desired settings
    bool ReapplyHDRColorCorrection(bool force);
    bool ReapplySDRColorCorrection(bool force);

    /**
     * Get config manager for direct access to settings
     * @return Pointer to ConfigManager
     */
    class ConfigManager* GetConfig() { return m_config; }

private:
    std::wstring GetExecutablePath() const;
    std::wstring GetToolPath(const wchar_t* toolName) const;
    std::wstring GetProfilePath(const wchar_t* profileName) const;

    bool ExtractEmbeddedResource(int resourceId, const wchar_t* resourceType, const std::wstring& outputPath);
    bool ExtractEmbeddedTools();
    void CleanupTemporaryFiles();

    bool ExecuteCommand(const std::wstring& command) const;
    bool ExecuteCommandWithOutput(const std::wstring& command, std::wstring& output) const;
    bool LoadICCProfile(const wchar_t* profilePath) const;
    bool SetMonitorVCP(int display, int vcpCode, int value) const;
    bool GetMonitorVCP(int display, int vcpCode, int& currentValue) const;
    bool SetMonitorVCPVerified(int display, int vcpCode, int value, int maxRetries = 3) const;
    bool WaitForVcpReadable(int display, int vcpCode, int timeoutMs, int pollMs) const;
    void Sleep(int milliseconds) const;

    // Paths
    std::wstring m_executablePath;
    std::wstring m_binPath;
    std::wstring m_profilesPath;
    std::wstring m_tempPath;

    // Tool paths
    std::wstring m_dispwinPath;
    std::wstring m_winddcutilPath;

    // Flags
    bool m_toolsExtracted;
    bool m_useEmbeddedTools;

    // Configuration from INI file
    class ConfigManager* m_config;
};

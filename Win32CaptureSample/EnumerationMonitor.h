#pragma once

struct EnumerationMonitor
{
public:
    static std::vector<EnumerationMonitor> EnumerateAllMonitors();

    HMONITOR Hmon() const noexcept { return m_hmon; }
    std::wstring DisplayName() const noexcept { return m_displayName; }

private:
    EnumerationMonitor(HMONITOR hmon, const std::wstring& displayName) : m_hmon(hmon), m_displayName(displayName)
    {
    }

    const HMONITOR m_hmon;
    const std::wstring m_displayName;
};
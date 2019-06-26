#pragma once

struct EnumerationMonitor
{
public:
    EnumerationMonitor(nullptr_t) {}
    EnumerationMonitor(HMONITOR hmon, std::wstring& displayName)
    {
        m_hmon = hmon;
        m_displayName = displayName;
    }

    HMONITOR Hmon() const noexcept { return m_hmon; }
    std::wstring DisplayName() const noexcept { return m_displayName; }

    static std::vector<EnumerationMonitor> EnumerateAllMonitors();

private:
    HMONITOR m_hmon;
    std::wstring m_displayName;
};
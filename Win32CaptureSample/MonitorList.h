#pragma once

struct MonitorInfo
{
    MonitorInfo(HMONITOR monitorHandle)
    {
        MonitorHandle = monitorHandle;
        MONITORINFOEX monitorInfo = { sizeof(monitorInfo) };
        winrt::check_bool(GetMonitorInfo(MonitorHandle, &monitorInfo));
        std::wstring displayName(monitorInfo.szDevice);
        DisplayName = displayName;
    }
    MonitorInfo(HMONITOR monitorHandle, std::wstring const& displayName)
    {
        MonitorHandle = monitorHandle;
        DisplayName = displayName;
    }

    HMONITOR MonitorHandle;
    std::wstring DisplayName;

    bool operator==(const MonitorInfo& monitor) { return MonitorHandle == monitor.MonitorHandle; }
    bool operator!=(const MonitorInfo& monitor) { return !(*this == monitor); }
};

class MonitorList
{
public:
    MonitorList(bool includeAllMonitors);
    
    void Update();
    void RegisterComboBoxForUpdates(HWND comboBoxHandle) { m_comboBoxes.push_back(comboBoxHandle); ForceUpdateComboBox(comboBoxHandle); }
    void UnregisterComboBox(HWND comboBoxHandle) { m_comboBoxes.erase(std::remove(m_comboBoxes.begin(), m_comboBoxes.end(), comboBoxHandle), m_comboBoxes.end()); }
    const std::vector<MonitorInfo> GetCurrentMonitors() { return m_monitors; }

private:
    void ForceUpdateComboBox(HWND comboBoxHandle);

private:
    std::vector<HWND> m_comboBoxes;
    std::vector<MonitorInfo> m_monitors;
    bool m_includeAllMonitors = false;
};
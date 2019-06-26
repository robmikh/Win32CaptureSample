#include "pch.h"
#include "EnumerationMonitor.h"

BOOL CALLBACK EnumDisplayMonitorsProc(HMONITOR hmon, HDC, LPRECT, LPARAM lparam)
{
    MONITORINFOEX monitorInfo = {};
    monitorInfo.cbSize = sizeof(MONITORINFOEX);
    if (!GetMonitorInfo(hmon, &monitorInfo))
    {
        winrt::check_hresult(GetLastError());
    }

    std::wstring displayName(monitorInfo.szDevice);

    std::vector<EnumerationMonitor>& monitors = *reinterpret_cast<std::vector<EnumerationMonitor>*>(lparam);
    monitors.push_back(EnumerationMonitor(hmon, displayName));

    return TRUE;
}

const std::vector<EnumerationMonitor> EnumerateMonitors()
{
    std::vector<EnumerationMonitor> monitors;
    EnumDisplayMonitors(NULL, NULL, EnumDisplayMonitorsProc, reinterpret_cast<LPARAM>(&monitors));
    return monitors;
}

std::vector<EnumerationMonitor> EnumerationMonitor::EnumerateAllMonitors()
{
    return EnumerateMonitors();
}
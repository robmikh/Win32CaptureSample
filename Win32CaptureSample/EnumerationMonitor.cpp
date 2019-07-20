#include "pch.h"
#include "EnumerationMonitor.h"

std::vector<EnumerationMonitor> EnumerationMonitor::EnumerateAllMonitors()
{
    std::vector<EnumerationMonitor> monitors;
    EnumDisplayMonitors(nullptr, nullptr, [](HMONITOR hmon, HDC, LPRECT, LPARAM lparam)
    {
        MONITORINFOEX monitorInfo = { sizeof(monitorInfo) };
        if (!GetMonitorInfoW(hmon, &monitorInfo))
        {
            winrt::check_hresult(GetLastError());
        }

        std::wstring displayName(monitorInfo.szDevice);

        auto& monitors = *reinterpret_cast<std::vector<EnumerationMonitor>*>(lparam);
        monitors.push_back({ hmon, displayName });

        return TRUE;
    }, reinterpret_cast<LPARAM>(&monitors));
    return monitors;
}
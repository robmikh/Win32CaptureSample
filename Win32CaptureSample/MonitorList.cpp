#include "pch.h"
#include "MonitorList.h"

std::vector<MonitorInfo> EnumerateAllMonitors(bool includeAllMonitors)
{
    std::vector<MonitorInfo> monitors;
    EnumDisplayMonitors(nullptr, nullptr, [](HMONITOR hmon, HDC, LPRECT, LPARAM lparam)
    {
        auto& monitors = *reinterpret_cast<std::vector<MonitorInfo>*>(lparam);
        monitors.push_back(MonitorInfo(hmon));

        return TRUE;
    }, reinterpret_cast<LPARAM>(&monitors));
    if (monitors.size() > 1 && includeAllMonitors)
    {
        monitors.push_back(MonitorInfo(nullptr, L"All Displays"));
    }
    return monitors;
}

MonitorList::MonitorList(bool includeAllMonitors)
{
    m_includeAllMonitors = includeAllMonitors;
    m_monitors = EnumerateAllMonitors(m_includeAllMonitors);
}

void MonitorList::Update()
{
    auto monitors = EnumerateAllMonitors(m_includeAllMonitors);
    std::map<HMONITOR, MonitorInfo> newMonitors;
    for (auto& monitor : monitors)
    {
        newMonitors.insert({ monitor.MonitorHandle, monitor });
    }

    std::vector<int> monitorIndexesToRemove;
    auto index = 0;
    for (auto& monitor : m_monitors)
    {
        auto search = newMonitors.find(monitor.MonitorHandle);
        if (search == newMonitors.end())
        {
            monitorIndexesToRemove.push_back(index);
        }
        else
        {
            newMonitors.erase(search);
        }
        index++;
    }

    // Remove old monitors
    std::sort(monitorIndexesToRemove.begin(), monitorIndexesToRemove.end(), std::greater<int>());
    for (auto& removalIndex : monitorIndexesToRemove)
    {
        m_monitors.erase(m_monitors.begin() + removalIndex);
        for (auto& comboBox : m_comboBoxes)
        {
            winrt::check_hresult(static_cast<const int32_t>(SendMessageW(comboBox, CB_DELETESTRING, removalIndex, 0)));
        }
    }

    // Add new monitors
    for (auto& pair : newMonitors)
    {
        auto monitor = pair.second;
        m_monitors.push_back(monitor);
        for (auto& comboBox : m_comboBoxes)
        {
            winrt::check_hresult(static_cast<const int32_t>(SendMessageW(comboBox, CB_ADDSTRING, 0, (LPARAM)monitor.DisplayName.c_str())));
        }
    }
}

void MonitorList::ForceUpdateComboBox(HWND comboBoxHandle)
{
    winrt::check_hresult(static_cast<const int32_t>(SendMessageW(comboBoxHandle, CB_RESETCONTENT, 0, 0)));
    for (auto& monitor : m_monitors)
    {
        winrt::check_hresult(static_cast<const int32_t>(SendMessageW(comboBoxHandle, CB_ADDSTRING, 0, (LPARAM)monitor.DisplayName.c_str())));
    }
}